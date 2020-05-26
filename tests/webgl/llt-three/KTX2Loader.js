/**
 * @author donmccurdy / https://www.donmccurdy.com
 * @author MarkCallow / https://github.com/MarkCallow
 *
 * References:
 * - KTX: http://github.khronos.org/KTX-Specification/
 * - DFD: https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#basicdescriptor
 */

import {
	CompressedTexture,
	CompressedTextureLoader,
	FileLoader,
	LinearEncoding,
	LinearFilter,
	LinearMipmapLinearFilter,
	RGB_ETC1_Format,
	RGB_ETC2_Format,
	RGBA_ETC2_EAC_Format,
	RGB_PVRTC_4BPPV1_Format,
	RGB_S3TC_DXT1_Format,
	RGBA_ASTC_4x4_Format,
	RGBA_PVRTC_4BPPV1_Format,
	RGBA_S3TC_DXT5_Format,
	sRGBEncoding,
	UnsignedByteType,
} from 'https://unpkg.com/three@0.114.0/build/three.module.js';


	const DFD = {
		modelUastc: 166,
		modelEtc1s: 163,
		channelUastc: {
			rgb: 0,
			rgba: 3,
			rrr: 4,
			rrrg: 5
		},
		channelEtc1s: {
			rgb: 0,
			rrr: 3,
			ggg: 4,
			aaa: 15,
		}
	};

class KTX2Loader extends CompressedTextureLoader {

	constructor ( manager ) {

		super( manager );

		this.basisModule = null;

		this.transcoderConfig = {};

	}

	detectSupport ( renderer ) {

		this.transcoderConfig = {
			astcSupported: !! renderer.extensions.get( 'WEBGL_compressed_texture_astc' ),
			etc1Supported: !! renderer.extensions.get( 'WEBGL_compressed_texture_etc1' ),
			etc2Supported: !! renderer.extensions.get( 'WEBGL_compressed_texture_etc' ),
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

				basisModule.initTranscoders();

				resolve();

			} );

		} );

	}

	load ( url, onLoad, onProgress, onError ) {

		var scope = this;

		var texture = new CompressedTexture();

		var bufferPending = new Promise( function (resolve, reject ) {

			new FileLoader( scope.manager )
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

		var BasisLzEtc1sImageTranscoder = this.basisModule.BasisLzEtc1sImageTranscoder;
		var UastcImageTranscoder = this.basisModule.UastcImageTranscoder;
		var TextureFormat = this.basisModule.TextureFormat;

		var ktx = new KTX2Container( buffer );

// TODO(donmccurdy): Should test if texture is transcodable before attempting
// any transcoding. If supercompressionScheme is KTX_SS_BASIS_LZ and dfd
// colorModel is ETC1S (163) or if dfd colorModel is UASTCF (166)
// then texture must be transcoded.

		var texFormat = ktx.dfd.colorModel == DFD.modelUastc ? TextureFormat.UASTC4x4
												   : TextureFormat.ETC1S;
		// TODO(donmccurdy): Handle all channelIds (i.e. the R & R+G cases),
		// choosing appropriate transcode target formats or providing queries
		// for applications so they know what to do with the content.
		var hasAlpha = false;
		if (texFormat == TextureFormat.UASTC4x4) {
			var transcoder = new UastcImageTranscoder();
			if ( (ktx.dfd.samples[0].channelId & 0xf) === DFD.channelUastc.rgba )
				hasAlpha = true;
		} else {
			var transcoder = new BasisLzEtc1sImageTranscoder();
			if ( ktx.dfd.numSamples == 2 && (ktx.dfd.samples[1].channelId & 0xf) === DFD.channelEtc1s.aaa ) {
				hasAlpha = true;
			}
		}
		ktx.initMipmaps( texFormat, hasAlpha, transcoder, this.basisModule, this.transcoderConfig )
			.then( function () {

				var texture = new CompressedTexture(
					ktx.mipmaps,
					ktx.getWidth(),
					ktx.getHeight(),
					ktx.transcodedFormat,
					UnsignedByteType
				);

				texture.encoding = ktx.getEncoding();
				texture.premultiplyAlpha = ktx.getPremultiplyAlpha();
				texture.minFilter = ktx.mipmaps.length === 1 ? LinearFilter : LinearMipmapLinearFilter;
				texture.magFilter = LinearFilter;

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

			this.levels.push( {
				byteOffset: levelReader.nextUint64(),
				byteLength: levelReader.nextUint64(),
				uncompressedByteLength: levelReader.nextUint64()
			} );

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

		const sampleStart = 6;
		const sampleWords = 4;
		this.dfd = {

			vendorId: dfdReader.skip( 4 /* totalSize */ ).nextUint16(),
			versionNumber: dfdReader.skip( 2 /* descriptorType */ ).nextUint16(),
			descriptorBlockSize: dfdReader.nextUint16(),
			colorModel: dfdReader.nextUint8(),
			colorPrimaries: dfdReader.nextUint8(),
			transferFunction: dfdReader.nextUint8(),
			flags: dfdReader.nextUint8(),
			texelBlockDimension: {
				x: dfdReader.nextUint8() + 1,
				y: dfdReader.nextUint8() + 1,
				z: dfdReader.nextUint8() + 1,
				w: dfdReader.nextUint8() + 1,
			},
			/*
			texelBlockDimension0: dfdReader.nextUint8(),
			texelBlockDimension1: dfdReader.nextUint8(),
			texelBlockDimension2: dfdReader.nextUint8(),
			*/
			bytesPlane0: dfdReader.nextUint8(),
			numSamples: 0,
			samples: [],
		};

			dfdReader.skip( 7 /* bytesPlane[1-7] */ );
			this.dfd.numSamples = (this.dfd.descriptorBlockSize/4 - sampleStart) / sampleWords;
		for (i = 0; i < this.dfd.numSamples; i++) {
			this.dfd.samples[i] = {
				channelId: dfdReader.skip( 3 /* bitOffset + bitLength */ ).nextUint8(),
				// ... remainder not implemented.
			};
			dfdReader.skip( 12 /* samplePosition[0-3], lower, upper */ );
		}

		if ( this.header.vkFormat !== 0x00 /* VK_FORMAT_UNDEFINED */ &&
			 !( this.header.supercompressionScheme === 1 /* BasisLZ */ ||
				this.dfd.colorModel === DFD.modelUastc ) ) {

			throw new Error( 'THREE.KTX2Loader: Only Basis Universal supercompression is currently supported.' );

		}

		///////////////////////////////////////////////////
		// Key/Value Data (KVD)
		///////////////////////////////////////////////////

		// Not implemented.
		this.kvd = {};


		///////////////////////////////////////////////////
		// Supercompression Global Data (SGD)
		///////////////////////////////////////////////////

		if (this.header.sgdByteLength > 0) {
			var sgdReader = new KTX2BufferReader(
				this.arrayBuffer,
				this.header.sgdByteOffset,
				this.header.sgdByteLength,
				littleEndian
			);

			this.sgd = {

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

			for ( var i = 0; i < this.header.levelCount; i ++ ) {

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
		} else {
		  this.sgd = {};
		}
	}

	initMipmaps ( texFormat, hasAlpha, transcoder, basisModule, config ) {

		var TranscodeTarget = basisModule.TranscodeTarget;
		var TextureFormat = basisModule.TextureFormat;
		var ImageInfo = basisModule.ImageInfo;

		var scope = this;

		var mipmaps = [];
		var width = this.getWidth();
		var height = this.getHeight();

		var isVideo = false;

		// For both ETC1S and UASTC4x4 formats.
		if ( texFormat == TextureFormat.ETC1S ) {
			var numEndpoints = this.sgd.endpointCount;
			var numSelectors = this.sgd.selectorCount;

			var endpoints = this.sgd.endpointsData;
			var selectors = this.sgd.selectorsData;
			var tables = this.sgd.tablesData;

			transcoder.decodePalettes( numEndpoints, endpoints, numSelectors, selectors );
			transcoder.decodeTables( tables );
		}
		var targetFormat;

		if ( config.astcSupported ) {

			targetFormat = TranscodeTarget.ASTC_4x4_RGBA;
			thights.transcodedFormat = RGBA_ASTC_4x4_Format;

		} else if ( config.dxtSupported ) {

			targetFormat = hasAlpha ? TranscodeTarget.BC3_RGBA : TranscodeTarget.BC1_RGB;
			this.transcodedFormat = hasAlpha ? RGBA_S3TC_DXT5_Format : RGB_S3TC_DXT1_Format;

		} else if ( config.pvrtcSupported ) {

			targetFormat = hasAlpha ? TranscodeTarget.PVRTC1_4_RGBA : TranscodeTarget.PVRTC1_4_RGB;
			this.transcodedFormat = hasAlpha ? RGBA_PVRTC_4BPPV1_Format : RGB_PVRTC_4BPPV1_Format;

		} else if ( config.etc2Supported ) {

			targetFormat = hasAlpha ? TranscodeTarget.ETC2_RGBA : TranscodeTarget.ETC1_RGB /* subset of ETC2 */;
			this.transcodedFormat = hasAlpha ? RGBA_ETC2_EAC_Format : RGB_ETC2_Format;

		} else if ( config.etc1Supported ) {

			targetFormat = TranscodeTarget.ETC1_RGB;
			this.transcodedFormat = RGB_ETC1_Format;

		} else {

			throw new Error( 'THREE.KTX2Loader: No suitable compressed texture format found.' );

		}
		if ( !basisModule.isFormatSupported( targetFormat, texFormat ) ) {
			throw new Error( 'THREE.KTX2Loader: Selected texture format not supported by current msc_basis_transcoder.js build.' );
		}

		var imageDescIndex = 0;

		for ( var level = 0; level < this.header.levelCount; level ++ ) {

			var levelWidth = width / Math.pow( 2, level );
			var levelHeight = height / Math.pow( 2, level );

			var numImagesInLevel = 1; // TODO(donmccurdy): Support cubemaps, arrays and 3D.
			var imageOffsetInLevel = 0;

			var imageInfo = new ImageInfo(texFormat, levelWidth, levelHeight, level);
			var levelImageByteLength = imageInfo.numBlocksX * imageInfo.numBlocksY * this.dfd.bytesPlane0;

			for ( var imageIndex = 0; imageIndex < numImagesInLevel; imageIndex ++ ) {

				var result;
				if ( texFormat == TextureFormat.UASTC4x4 ) {
					imageInfo.flags = 0;
					imageInfo.rgbByteOffset = 0;
					imageInfo.rgbByteLength = levelImageByteLength;
					imageInfo.alphaByteOffset = 0;
					imageInfo.alphaByteLength = 0;
					result = transcoder.transcodeImage(targetFormat,
						new Uint8Array( this.arrayBuffer, this.levels[ level ].byteOffset + imageOffsetInLevel, levelImageByteLength),
						imageInfo,
						0,
						hasAlpha,
						isVideo
					);
				} else {
					var imageDesc = this.sgd.imageDescs[ imageDescIndex++ ];

					imageInfo.flags = imageDesc.imageFlags;
					imageInfo.rgbByteOffset = 0;
					imageInfo.rgbByteLength = imageDesc.rgbSliceByteLength;
					imageInfo.alphaByteOffset = imageDesc.alphaSliceByteOffset > 0 ? imageDesc.rgbSliceByteLength : 0;
					imageInfo.alphaByteLength = imageDesc.alphaSliceByteLength;
					result = transcoder.transcodeImage(targetFormat,
						new Uint8Array( this.arrayBuffer, this.levels[ level ].byteOffset + imageDesc.rgbSliceByteOffset, imageDesc.rgbSliceByteLength + imageDesc.alphaSliceByteLength ),
						imageInfo,
						0,
						isVideo
					);
				}

				if ( result.transcodedImage === undefined ) {

					throw new Error( 'THREE.KTX2Loader: Unable to transcode image.' );

				}

				// Transcoded image is written in memory allocated by WASM. We could avoid copying
				// the image by waiting until the image is uploaded to the GPU, then calling
				// delete(). However, (1) we don't know if the user will later need to re-upload it
				// e.g. after calling texture.clone(), and (2) this code will eventually be in a
				// Web Worker, and transferring WASM's memory seems like a very bad idea.
				var levelData = result.transcodedImage.get_typed_memory_view().slice();
				result.transcodedImage.delete();

				mipmaps.push( { data: levelData, width: levelWidth, height: levelHeight } );
				imageOffsetInLevel += levelImageByteLength;

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
			? sRGBEncoding
			: LinearEncoding;

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

export { KTX2Loader };
