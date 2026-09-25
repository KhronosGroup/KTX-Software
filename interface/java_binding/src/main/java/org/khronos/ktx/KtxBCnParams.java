/*
 * Copyright (c) 2026, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Structure for passing extended parameters to
 * {@link KtxTexture2#compressBCnEx(KtxBCnParams)}.<br>
 *
 * If you only want default values at a minimum you must initialize the
 * structure as follows:
 * <code>
 *  ktxBCnParams params = new ktxBCnParams();
 *  // You have to set the target BCn format:
 *  params.bcn = KtxBCnCompression.BCX;
 *  // And if targeting BC1, BC2, BC3, or BC7, you have to set quality level:
 *  params.bcnCompressionQuality = KtxPackBCnQualityLevels.XXXX;
 * </code>
 */
public class KtxBCnParams {

  /**
   * Number of threads used for compression and RDO. Default is 1.
   */
  private int threadCount;

  /**
   * BCn format to compress the uncompressed images to. Given as a
   * {@link KtxBCnCompression}. Only options related to the provided target BCn
   * format are used.
   *
   * Since BC7 encoding is performed using basisu's analytical encoder which
   * encodes so rapidly (on average), that apart from lower VRAM consumption
   * (4bpp vs. 8bpp) and better GPU texture cache efficiency, there's little
   * need to use BC1 now. BC3 still has an advantage vs. BC7, because it very
   * strongly separates how RGB is encoded from the alpha channel, in a
   * predictable way.
   */
  private int bcn;

  /**
   * BC1 (consequently BC2 and BC3) and BC7 compression quality. Supplied as a
   * value of {@link KtxPackBCnQualityLevels}. Lower values give faster
   * compression speed but potentially lower quality. Higher values give slower
   * compression speed but potentially better quality. There is no default.
   * Caller must explicitly set this value.
   *
   * For BC1, BC2, and BC3, this maps to the range [0, 19].
   * For BC7, this maps to an OR'ed set of low-level flags.
   */
  private int bcnCompressionQuality;

  /**
   * RDO quality scalar (lambda). Controls rate vs. distortion tradeoff. Lower
   * values yield higher quality/larger LZ compressed files, higher values yield
   * lower quality/smaller LZ compressed files. A good range to try is [0.25,8].
   * Full range is [.001,50.0]. Default is 1.0.
   * The post-processor tries to minimize:
   * distortion * smooth_block_scale + rate * lambda
   * (rate is approximate LZ bits and distortion is scaled MSE multiplied by the
   * smooth block MSE weighting factor). Larger values push the post-processor
   * towards optimizing more for lower rate, and smaller values more for
   * distortion.
   *
   * Currently, HDR formats (i.e., BC6HU/BC6HS) are not supported.
   */
  private float bcnRDOQualityScalar;

  /**
   * The number of bytes up to which the encoder can look back to from each
   * block to find matches. The larger this value, the slower the encoder but
   * the higher the quality per LZ compressed bit. Range is [64,65536].
   * Default is 4096.
   */
  private int bcnRDODictSize;

  /**
   * RDO max MSE scaling factor for blocks considered to be smooth/flat. A value
   * of 1.0 means no smooth block error scaling which may cause very noticeable
   * artifacts for smooth/flat blocks (e.g., kodim23 test image).
   *
   * @e bcnRDOMaxSmoothBlockStdDev is used to compute, for a given block, the
   * MSE scale factor in the range: 1.0 (i.e., not a smooth block) up to this
   * max MSE scale factor.
   *
   * As to why an MSE factor has to be applied to smooth/flat blocks, the MSE
   * for these blocks is too low relative to the visual impact they have when
   * they get distorted. The solution implemented here is to compute the max std
   * dev. of any component and use a linear function of that to scale
   * block/trial MSE.
   *
   * Range is [1,300]. Default is to automatically compute a decent conservative
   * smooth block MSE max scaling factor.
   *
   * If this is set to 0 (default), automatically compute a decent conservative
   * smooth block MSE max scaling factor. There is no single calculation/set of
   * settings that work perfectly on all input textures, but the formula in the
   * code works OK for most textures at low-ish lambdas (For an example of a
   * difficult texture the currently formulas/settings doesn't handle so well,
   * try encoding kodim03 at lambdas 1-3). Smooth block handling is tuned so
   * lambdas at or near 1 look OK on textures with smooth gradients, skies, etc.
   */
  private float bcnRDOMaxSmoothBlockErrorScale;

  /**
   * RDO max smooth/flat block standard deviation. If the std dev. of a block
   * exceeds this value, then it won't be considered as a smooth block (i.e.,
   * the smooth block MSE scale factor will be set to 1 for this block). The
   * smaller the ratio of the std dev. of this block to this value the more the
   * smooth block MSE scale factor approaches @p bcnRDOMaxSmoothBlockErrorScale.
   * Range is [.01,65536.0]. Larger values expand the range of blocks considered
   * smooth. Default is 18.0.
   */
  private float bcnRDOMaxSmoothBlockStdDev;

  /**
   * How much the RMS error of a block is allowed to increase before a trial is
   * rejected. 1.0=no increase allowed, 1.05=5% increase allowed, etc. Range is
   * [1.001, 100.0]. Default is 10.0.
   */
  private float bcnRDOMaxAllowedRMSIncreaseRatio;

  /**
   * Enable Rate Distortion Optimization (RDO) post-processing step on
   * BCn-encoded blocks to reduce entropy with Deflate/LZMA/LZHAM optimizations.
   * This is primarily used to reduce size on disk by applying a further
   * compression, mainly: Deflate, LZMA, or LZHAM. RDO parameters are only used
   * if this is set. Setting this might result in significantly slower encoding
   * time at the benefit of potentially significantly lower bit rate (i.e.,
   * number of bits per encoded texel). Default is false.
   */
  private boolean bcnRDO;

  /**
   * Disable the detection of extremely smooth blocks and encoding them with a
   * significantly higher MSE scale factor. When disabled, a per-block mask
   * image is computed, filtered, then an array of per-block MSE scale factors
   * is supplied to the ERT. The end result is significantly less artifacts on
   * regions containing very smooth blocks (e.g., gradients, faded background,
   * skies, etc.). Enabling ultrasmooth block handling hurts rate-distortion
   * performance. Default is false.
   *
   * This only applies to BC1, BC3, and BC7's RGB blocks (alpha is ignored). For
   * other formats, this is silently ignored.
   */
  private boolean bcnRDONoUltrasmoothBlockHandling;

  /**
   * If disabled, inject up to 2 matches into each block as opposed to just one
   * match. Enabling this results in faster but but noticeably lower
   * compression. Default is false.
   */
  private boolean bcnRDOTryOneMatch;

  /**
   * Skip blocks that have zero mean-squared error (MSR). Might result in faster
   * compression speed but potentially lower compression. Default is false.
   */
  private boolean bcnRDOSkipZeroMSEBlocks;

  /**
   * Disable RDO multithreading (potentially slightly higher compression).
   * Default is false.
   */
  private boolean bcnRDONoMultithreading;

  /**
   * Get number of threads used for compression and RDO.
   *
   * @return The number of threads
   */
  public int getThreadCount() {
    return threadCount;
  }

  /**
   * Set number of threads used for compression and RDO. Default is 1.
   *
   * @param threadCount The number of threads
   */
  public void setThreadCount(int threadCount) {
    this.threadCount = threadCount;
  }

  /**
   * Get BCn compression format to compress the uncompressed images to.
   *
   * @return BCn compression format as a value of {@link KtxBCnCompression}
   */
  public int getBCn() {
    return bcn;
  }

  /**
   * Set BCn compression format to compress the uncompressed images to.
   *
   * @param bcn BCn compression format as a value of {@link KtxBCnCompression}
   */
  public void setBCn(int bcn) {
    this.bcn = bcn;
  }

  /**
   * Get BC1 (consequently BC2 and BC3) and BC7 compression quality.
   *
   * @return BC1/BC2/BC3 and BC7 compression quality level as a value of
   * {@link KtxPackBCnQualityLevels}.
   */
  public int getBCnCompressionQuality() {
    return bcnCompressionQuality;
  }

  /**
   * Set BC1 (consequently BC2 and BC3) and BC7 compression quality.
   *
   * @param bcnCompressionQuality BC1/BC2/BC3 and BC7 compression quality (can
   * be supplied as a value of {@link KtxBCnCompression})
   */
  public void setBCnCompressionQuality(int bcnCompressionQuality) {
    this.bcnCompressionQuality = bcnCompressionQuality;
  }

  /**
   * Get RDO quality scalar (lambda).
   *
   * @return RDO lambda.
   */
  public float getBCnRDOQualityScalar() {
    return bcnRDOQualityScalar;
  }

  /**
   * Set RDO quality scalar (lambda). Default is 1.0.
   *
   * @param bcnRDOQualityScalar RDO lambda. Full range is [.001,50.0].
   */
  public void setBCnRDOQualityScalar(float bcnRDOQualityScalar) {
    this.bcnRDOQualityScalar = bcnRDOQualityScalar;
  }

  /**
   * Get RDO dictionary (i.e., lookback window) size in bytes.
   *
   * @return RDO dictionary size in bytesh
   */
  public int getBCnRDODictSize() {
    return bcnRDODictSize;
  }

  /**
   * Set RDO dictionary size. Default is 4096.
   *
   * @param bcnRDODictSize dictionary size in bytes. Range is [64,65536].
   */
  public void setBCnRDODictSize(int bcnRDODictSize) {
    this.bcnRDODictSize = bcnRDODictSize;
  }
  
  /**
   * Get RDO max MSE scaling factor for blocks considered to be smooth/flat.
   *
   * @return RDO mas MSE scaling factor
   */
  public float getBCnRDOMaxSmoothBlockErrorScale() {
    return bcnRDOMaxSmoothBlockErrorScale;
  }

  /**
   * Set RDO max MSE scaling factor for blocks considered to be smooth/flat.
   * Default is 0 (i.e., automatically compute a decent conservative smooth
   * block MSE max scaling factor).
   *
   *
   * @param bcnRDOMaxSmoothBlockErrorScale RDO max MSE scaling factor.
   * Range is [1,300]. 
   */
  public void setBCnRDOMaxSmoothBlockErrorScale(float bcnRDOMaxSmoothBlockErrorScale) {
    this.bcnRDOMaxSmoothBlockErrorScale = bcnRDOMaxSmoothBlockErrorScale;
  }

  /**
   * Get RDO max smooth/flat block standard deviation.
   *
   * @return RDO max smooth/flat block standard deviation.
   */
  public float getBCnRDOMaxSmoothBlockStdDev() {
    return bcnRDOMaxSmoothBlockStdDev;
  }

  /**
   * Set RDO max smooth/flat block standard deviation. Default is 18.0.
   *
   * @param bcnRDOMaxSmoothBlockStdDev RDO max smooth block std dev.
   * Range is [.01,65536.0]. Larger values expand the range of blocks considered
   * smooth.
   */
  public void setBCnRDOMaxSmoothBlockStdDev(float bcnRDOMaxSmoothBlockStdDev) {
    this.bcnRDOMaxSmoothBlockStdDev = bcnRDOMaxSmoothBlockStdDev;
  }

  /**
   * Get how much the RMS error of a block is allowed to increase before a trial
   * is rejected.
   *
   * @return RMS error
   */
  public float getBCnRDOMaxAllowedRMSIncreaseRatio() {
    return bcnRDOMaxAllowedRMSIncreaseRatio;
  }

  /**
   * Set how much the RMS error of a block is allowed to increase before a trial
   * is rejected. Range is [1.001, 100.0]. Default is 10.0.
   *
   * @param bcnRDOMaxAllowedRMSIncreaseRatio RMS error. Range is [1.001, 100.0]. 
   */
  public void setBCnRDOMaxAllowedRMSIncreaseRatio(float bcnRDOMaxAllowedRMSIncreaseRatio) {
    this.bcnRDOMaxAllowedRMSIncreaseRatio = bcnRDOMaxAllowedRMSIncreaseRatio;
  }

  /**
   * Get whether RDO is enabled or not.
   *
   * @return whether RDO is enabled.
   */
  public boolean getBCnRDO() {
    return bcnRDO;
  }

  /**
   * Set Rate Distortion Optimization (RDO) post-processing step on BCn-encoded
   * blocks to reduce entropy with Deflate/LZMA/LZHAM optimizations.
   *
   * @param bcnRDO whether to enable RDO.
   */
  public void setBCnRDO(boolean bcnRDO) {
    this.bcnRDO = bcnRDO;
  }

  /**
   * Get whether to disable the detection of extremely smooth blocks and
   * encoding them with a significantly higher MSE scale factor.
   *
   * @return whether ultrasmooth block handling is disabled.
   */
  public boolean getBCnRDONoUltrasmoothBlockHandling() {
    return bcnRDONoUltrasmoothBlockHandling;
  }

  /**
   * Set whether to disable the detection of extremely smooth blocks and
   * encoding them with a significantly higher MSE scale factor.
   *
   * @param bcnRDONoUltrasmoothBlockHandling whether to disable ultrasmooth
   * block handling.
   */
  public void setBCnRDONoUltrasmoothBlockHandling(boolean bcnRDONoUltrasmoothBlockHandling) {
    this.bcnRDONoUltrasmoothBlockHandling = bcnRDONoUltrasmoothBlockHandling;
  }

  /**
   * Get whether to disable the injection of up to 2 matches into each block as
   * opposed to just one match.
   *
   * @return whether one-match mode is enabled
   */
  public boolean getBCnRDOTryOneMatch() {
    return bcnRDOTryOneMatch;
  }

  /**
   * Set whether to disable the injection of up to 2 matches into each block as
   * opposed to just one match.
   *
   * @param bcnRDOTryOneMatch whether one-match mode is enabled
   */
  public void setBCnRDOTryOneMatch(boolean bcnRDOTryOneMatch) {
    this.bcnRDOTryOneMatch = bcnRDOTryOneMatch;
  }

  /**
   * Get whether to skip blocks that have zero mean-squared error (MSR).
   *
   * @return whether to skip zero-MSE blocks
   */
  public boolean getBCnRDOSkipZeroMSEBlocks() {
    return bcnRDOSkipZeroMSEBlocks;
  }

  /**
   * Set whether to skip blocks that have zero mean-squared error (MSR).
   *
   * @param bcnRDOSkipZeroMSEBlocks whether to skip zero-MSE blocks
   */
  public void set(boolean bcnRDOSkipZeroMSEBlocks) {
    this.bcnRDOSkipZeroMSEBlocks = bcnRDOSkipZeroMSEBlocks;
  }

  /**
   * Get whether to disable RDO multithreading (potentially slightly higher
   * compression).
   *
   * @return whether RDO multithreading is disabled.
   */
  public boolean getBCnRDONoMultithreading() {
    return bcnRDONoMultithreading;
  }

  /**
   * Set whether to disable RDO multithreading (potentially slightly higher
   * compression).
   *
   * @param bcnRDONoMultithreading whether RDO multithreading is disabled.
   */
  public void setBCnRDONoMultithreading(boolean bcnRDONoMultithreading) {
    this.bcnRDONoMultithreading = bcnRDONoMultithreading;
  }
}
