/**
 * @author donmccurdy / https://www.donmccurdy.com
 * @author MarkCallow / https://github.com/MarkCallow
 *
 * References:
 * - KTX: http://github.khronos.org/KTX-Specification/
 * - DFD: https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#basicdescriptor
 */

class KTX2Loader extends THREE.CompressedTextureLoader {

	constructor ( manager ) {

		super( manager );

		this.basisModule = null;

		this.transcoderConfig = {};

	}

	detectSupport ( renderer ) {

		this.transcoderConfig = {
			astcSupported: !! renderer.extensions.get( 'WEBGL_compressed_texture_astc' ),
			etcSupported: !! renderer.extensions.get( 'WEBGL_compressed_texture_etc1' ),
			dxtSupported: !! renderer.extensions.get( 'WEBGL_compressed_texture_s3tc' ),
			pvrtcSupported: !! renderer.extensions.get( 'WEBGL_compressed_texture_pvrtc' )
				|| !! renderer.extensions.get( 'WEBKIT_WEBGL_compressed_texture_pvrtc' )
		};

		return this;

	}

	init () {

		var scope = this;

		// The Emscripten wrapper returns a fake Promise, which can cause
		// infinite recursion when mixed with native Promises. Wrap the module
		// initialization to return a native Promise.
		return new Promise( function ( resolve ) {

			MSC_TRANSCODER().then( function ( basisModule ) {

				scope.basisModule = basisModule;

				resolve();

			} );

		} );

	}

	load ( url, onLoad, onProgress, onError ) {

		var scope = this;

		var texture = new THREE.CompressedTexture();

		var bufferPending = new Promise( function (resolve, reject ) {

			new THREE.FileLoader( scope.manager )
				.setPath( scope.path )
				.setResponseType( 'arraybuffer' )
				.load( url, resolve, onProgress, reject );

		} );

		Promise.all( [ bufferPending, this.init() ] ).then( function ( [ buffer ] ) {

			scope.parse( buffer, function ( _texture ) {

				texture.copy( _texture );
				texture.needsUpdate = true;

				if ( onLoad ) onLoad( texture );

			}, onError );

		} );

		return texture;

	}

	parse ( buffer, onLoad, onError ) {

		var BasisTranscoder = this.basisModule.BasisTranscoder;

		BasisTranscoder.init();

		var ktx = new KTX2Container( buffer );
		var transcoder = new BasisTranscoder();

		ktx.initMipmaps( transcoder, this.basisModule, this.transcoderConfig )
			.then( function () {

				var texture = new THREE.CompressedTexture(
					ktx.mipmaps,
					ktx.getWidth(),
					ktx.getHeight(),
					ktx.transcodedFormat,
					THREE.UnsignedByteType
				);

				texture.encoding = ktx.getEncoding();
				texture.premultiplyAlpha = ktx.getPremultiplyAlpha();
				texture.minFilter = ktx.mipmaps.length === 1 ? THREE.LinearFilter : THREE.LinearMipmapLinearFilter;
				texture.magFilter = THREE.LinearFilter;

				onLoad( texture );

			} )
			.catch( onError );

		return this;

	}

}

class KTX2Container {

	constructor ( arrayBuffer ) {

		this.arrayBuffer = arrayBuffer;

		this.mipmaps = null;
		this.transcodedFormat = null;

		// Confirm this is a KTX 2.0 file, based on the identifier in the first 12 bytes.
		var idByteLength = 12;
		var id = new Uint8Array( this.arrayBuffer, 0, idByteLength );
		if ( id[ 0 ] !== 0xAB || // '´'
				id[ 1 ] !== 0x4B ||  // 'K'
				id[ 2 ] !== 0x54 ||  // 'T'
				id[ 3 ] !== 0x58 ||  // 'X'
				id[ 4 ] !== 0x20 ||  // ' '
				id[ 5 ] !== 0x32 ||  // '2'
				id[ 6 ] !== 0x30 ||  // '0'
				id[ 7 ] !== 0xBB ||  // 'ª'
				id[ 8 ] !== 0x0D ||  // '\r'
				id[ 9 ] !== 0x0A ||  // '\n'
				id[ 10 ] !== 0x1A || // '\x1A'
				id[ 11 ] !== 0x0A    // '\n'
			) {

			throw new Error( 'THREE.KTX2Loader: Missing KTX 2.0 identifier.' );

		}

		// TODO(donmccurdy): If we need to support BE, derive this from typeSize.
		var littleEndian = true;


		///////////////////////////////////////////////////
		// Header.
		///////////////////////////////////////////////////

		var headerByteLength = 17 * Uint32Array.BYTES_PER_ELEMENT;
		var headerReader = new KTX2BufferReader( this.arrayBuffer, idByteLength, headerByteLength, littleEndian );

		this.header = {

			vkFormat: headerReader.nextUint32(),
			typeSize: headerReader.nextUint32(),
			pixelWidth: headerReader.nextUint32(),
			pixelHeight: headerReader.nextUint32(),
			pixelDepth: headerReader.nextUint32(),
			arrayElementCount: headerReader.nextUint32(),
			faceCount: headerReader.nextUint32(),
			levelCount: headerReader.nextUint32(),

			supercompressionScheme: headerReader.nextUint32(),

			dfdByteOffset: headerReader.nextUint32(),
			dfdByteLength: headerReader.nextUint32(),
			kvdByteOffset: headerReader.nextUint32(),
			kvdByteLength: headerReader.nextUint32(),
			sgdByteOffset: headerReader.nextUint64(),
			sgdByteLength: headerReader.nextUint64(),

		};

		if ( this.header.vkFormat !== 0x00 /* VK_FORMAT_UNDEFINED */ ||
			 this.header.supercompressionScheme !== 1 /* Basis */ ) {

			throw new Error( 'THREE.KTX2Loader: Only Basis Universal supercompression is currently supported.' );

		}

		if ( this.header.pixelDepth > 0 ) {

			throw new Error( 'THREE.KTX2Loader: Only 2D textures are currently supported.' );

		}

		if ( this.header.arrayElementCount > 1 ) {

			throw new Error( 'THREE.KTX2Loader: Array textures are not currently supported.' );

		}

		if ( this.header.faceCount > 1 ) {

			throw new Error( 'THREE.KTX2Loader: Cube textures are not currently supported.' );

		}


		///////////////////////////////////////////////////
		// Level index
		///////////////////////////////////////////////////

		var levelByteLength = this.header.levelCount * 3 * 8;
		var levelReader = new KTX2BufferReader( this.arrayBuffer, idByteLength + headerByteLength, levelByteLength, littleEndian );

		this.levels = [];

		for ( var i = 0; i < this.header.levelCount; i ++ ) {

			var level = {
				byteOffset: levelReader.nextUint64(),
				byteLength: levelReader.nextUint64(),
				uncompressedByteLength: levelReader.nextUint64()
			};

			level.bytes = new Uint8Array( this.arrayBuffer, level.byteOffset, level.byteLength );

			this.levels.push( level );

		}


		///////////////////////////////////////////////////
		// Data Format Descriptor (DFD)
		///////////////////////////////////////////////////

		var dfdReader = new KTX2BufferReader(
			this.arrayBuffer,
			this.header.dfdByteOffset,
			this.header.dfdByteLength,
			littleEndian
		);

		this.dfd = {

			vendorId: dfdReader.skip( 4 /* totalSize */ ).nextUint16(),
			versionNumber: dfdReader.skip( 2 /* descriptorType */ ).nextUint16(),
			colorModel: dfdReader.skip( 2 /* descriptorBlockSize */ ).nextUint8(),
			colorPrimaries: dfdReader.nextUint8(),
			transferFunction: dfdReader.nextUint8(),
			flags: dfdReader.nextUint8(),

			// ... remainder not implemented.

		};


		///////////////////////////////////////////////////
		// Key/Value Data (KVD)
		///////////////////////////////////////////////////

		// Not implemented.
		this.kvd = {};


		///////////////////////////////////////////////////
		// Supercompression Global Data (SGD)
		///////////////////////////////////////////////////

		var sgdReader = new KTX2BufferReader(
			this.arrayBuffer,
			this.header.sgdByteOffset,
			this.header.sgdByteLength,
			littleEndian
		);

		this.sgd = {

			globalFlags: sgdReader.nextUint32(),
			endpointCount: sgdReader.nextUint16(),
			selectorCount: sgdReader.nextUint16(),
			endpointsByteLength: sgdReader.nextUint32(),
			selectorsByteLength: sgdReader.nextUint32(),
			tablesByteLength: sgdReader.nextUint32(),
			extendedByteLength: sgdReader.nextUint32(),

			imageDescs: [],

			endpointsData: null,
			selectorsData: null,
			tablesData: null,
			extendedData: null,

		};

		for ( var j = 0; j < this.header.levelCount; j ++ ) {

			this.sgd.imageDescs.push( {

				imageFlags: sgdReader.nextUint32(),
				rgbSliceByteOffset: sgdReader.nextUint32(),
				rgbSliceByteLength: sgdReader.nextUint32(),
				alphaSliceByteOffset: sgdReader.nextUint32(),
				alphaSliceByteLength: sgdReader.nextUint32(),

			} );

		}

		var endpointsByteOffset = this.header.sgdByteOffset + sgdReader.offset;
		var selectorsByteOffset = endpointsByteOffset + this.sgd.endpointsByteLength;
		var tablesByteOffset = selectorsByteOffset + this.sgd.selectorsByteLength;
		var extendedByteOffset = tablesByteOffset + this.sgd.tablesByteLength;

		this.sgd.endpointsData = new Uint8Array( this.arrayBuffer, endpointsByteOffset, this.sgd.endpointsByteLength );
		this.sgd.selectorsData = new Uint8Array( this.arrayBuffer, selectorsByteOffset, this.sgd.selectorsByteLength );
		this.sgd.tablesData = new Uint8Array( this.arrayBuffer, tablesByteOffset, this.sgd.tablesByteLength );
		this.sgd.extendedData = new Uint8Array( this.arrayBuffer, extendedByteOffset, this.sgd.extendedByteLength );

	}

	initMipmaps ( transcoder, basisModule, config ) {

		var TranscodeTarget = basisModule.TranscodeTarget;

		var scope = this;

		var mipmaps = [];
		var width = this.getWidth();
		var height = this.getHeight();

		// TODO(donmccurdy): Is this available explicitly in DFD?
		var hasAlpha = !!this.sgd.imageDescs[ 0 ].alphaSliceByteLength;
		var isVideo = false;

		// For ETC1S Basis Universal block compression.
		var blockWidth = 4, blockHeight = 4;

		var numEndpoints = this.sgd.endpointCount;
		var numSelectors = this.sgd.selectorCount;

		var endpoints = this.sgd.endpointsData;
		var selectors = this.sgd.selectorsData;
		var tables = this.sgd.tablesData;

		transcoder.decodePalettes( numEndpoints, endpoints, numSelectors, selectors );
		transcoder.decodeTables( tables );

		var targetFormat;

		if ( config.astcSupported ) {

			targetFormat = TranscodeTarget.ASTC_4x4_RGBA;
			this.transcodedFormat = THREE.RGBA_ASTC_4x4_Format;

		} else if ( config.dxtSupported ) {

			targetFormat = hasAlpha ? TranscodeTarget.BC3_RGBA : TranscodeTarget.BC1_RGB;
			this.transcodedFormat = hasAlpha ? THREE.RGBA_S3TC_DXT5_Format : THREE.RGB_S3TC_DXT1_Format;

		} else if ( config.pvrtcSupported ) {

			targetFormat = hasAlpha ? TranscodeTarget.PVRTC1_4_RGBA : TranscodeTarget.PVRTC1_4_RGB;
			this.transcodedFormat = hasAlpha ? THREE.RGBA_PVRTC_4BPPV1_Format : THREE.THREE.RGB_PVRTC_4BPPV1_Format;

		} else if ( config.etcSupported ) {

			targetFormat = TranscodeTarget.ETC1_RGB;
			this.transcodedFormat = THREE.RGB_ETC1_Format;

		} else {

			throw new Error( 'THREE.KTX2Loader: No suitable compressed texture format found.' );

		}

		var imageDescIndex = 0;

		for ( var level = 0; level < this.header.levelCount; level ++ ) {

			var levelWidth = width / Math.pow( 2, level );
			var levelHeight = height / Math.pow( 2, level );

			var numBlocksX = Math.floor( ( levelWidth + ( blockWidth - 1 ) ) / blockWidth );
			var numBlocksY = Math.floor( ( levelHeight + ( blockHeight - 1 ) ) / blockHeight );

			var numImagesInLevel = 1; // TODO(donmccurdy): Support cubemaps.

			for ( var imageIndex = 0; imageIndex < numImagesInLevel; imageIndex ++ ) {

				var imageDesc = this.sgd.imageDescs[ imageDescIndex++ ];

				var result = transcoder.transcodeImage(
					imageDesc.imageFlags,
					new Uint8Array( this.levels[level].bytes, imageDesc.rgbSliceByteOffset, imageDesc.rgbSliceByteLength ),
					new Uint8Array( this.levels[level].bytes, imageDesc.alphaSliceByteOffset, imageDesc.alphaSliceByteLength ),
					targetFormat,
					level,
					levelWidth, levelHeight,
					numBlocksX,
					numBlocksY,
					isVideo,
					false
				);

				if ( result.error ) {

					throw new Error( 'THREE.KTX2Loader: Unable to transcode image.' );

				}

				mipmaps.push( { data: result.transcodedImage, width: levelWidth, height: levelHeight } );

			}

		}

		return new Promise( function ( resolve, reject ) {

			scope.mipmaps = mipmaps;

			resolve();

		} );

	}

	getWidth () { return this.header.pixelWidth; }

	getHeight () { return this.header.pixelHeight; }

	getEncoding () {

		return this.dfd.transferFunction === 2 /* KHR_DF_TRANSFER_SRGB */
			? THREE.sRGBEncoding
			: THREE.LinearEncoding;

	}

	getPremultiplyAlpha () {

		return !! ( this.dfd.flags & 1 /* KHR_DF_FLAG_ALPHA_PREMULTIPLIED */ );

	}

}

class KTX2BufferReader {

	constructor ( arrayBuffer, byteOffset, byteLength, littleEndian ) {

		this.dataView = new DataView( arrayBuffer, byteOffset, byteLength );
		this.littleEndian = littleEndian;
		this.offset = 0;

	}

	nextUint8 () {

		var value = this.dataView.getUint8( this.offset, this.littleEndian );

		this.offset += 1;

		return value;

	}

	nextUint16 () {

		var value = this.dataView.getUint16( this.offset, this.littleEndian );

		this.offset += 2;

		return value;

	}

	nextUint32 () {

		var value = this.dataView.getUint32( this.offset, this.littleEndian );

		this.offset += 4;

		return value;

	}

	nextUint64 () {

		// https://stackoverflow.com/questions/53103695/
		var left =  this.dataView.getUint32( this.offset, this.littleEndian );
		var right = this.dataView.getUint32( this.offset + 4, this.littleEndian );
		var value = this.littleEndian ? left + ( 2 ** 32 * right ) : ( 2 ** 32 * left ) + right;

		if ( ! Number.isSafeInteger( value ) ) {

			console.warn( 'THREE.KTX2Loader: ' + value + ' exceeds MAX_SAFE_INTEGER. Precision may be lost.' );

		}

		this.offset += 8;

		return value;

	}

	skip ( bytes ) {

		this.offset += bytes;

		return this;

	}

}

THREE.KTX2Loader = KTX2Loader;
