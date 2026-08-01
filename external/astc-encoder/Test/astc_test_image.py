#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2019-2026 Arm Limited
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy
# of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
# -----------------------------------------------------------------------------
'''
The image test runner is used for image quality and performance testing.

It is designed to process directories of arbitrary test images, using the
directory structure and path naming conventions to self-describe how each image
is to be compressed. Some built-in test sets are provided in the ./Test/Images
directory, and others can be downloaded by running the astc_test_image_dl
script.

Attributes:
    RESULT_THRESHOLD_WARN: The result threshold (dB) for getting a WARN.
    RESULT_THRESHOLD_FAIL: The result threshold (dB) for getting a FAIL.
    TEST_BLOCK_SIZES: The block sizes we can test. This is a subset of the
        block sizes supported by ASTC, simply to keep test run times
        manageable.
'''

import argparse
from pathlib import Path
import sys
from typing import Optional

import testlib.encoder as te
from testlib.testset import TestImage, TestSet
from testlib.resultset import Record, ResultSet, ResultStatus

# Require bit exact with reference scores
RESULT_THRESHOLD_WARN = -0.00
RESULT_THRESHOLD_FAIL = -0.00
RESULT_THRESHOLD_3D_FAIL = -0.00

TEST_BLOCK_SIZES = [
    '4x4', '5x5', '6x6', '8x8', '12x12', '3x3x3', '6x6x6'
]

TEST_QUALITIES = [
    'fastest', 'fast', 'medium', 'thorough', 'verythorough', 'exhaustive'
]


def is_3d(block_size: str) -> bool:
    '''
    Is the given block size a 3D block type?

    Args:
        block_size: The block size.

    Return:
        True if the block size if 3D, False if 2D.
    '''
    return block_size.count('x') == 2


def count_test_set(test_set: TestSet, block_sizes: list[str]) -> int:
    '''
    Count the number of test executions needed for a test set.

    Args:
        test_set: The test set to run.
        block_sizes: The block sizes to run.

    Return:
        The number of test executions needed.
    '''
    count = 0
    for block_size in block_sizes:
        for image in test_set.tests:
            # 3D block sizes require 3D images
            if is_3d(block_size) != image.is_3d:
                continue

            count += 1

    return count


def determine_result(image: TestImage, reference: Record,
                     result: Record) -> ResultStatus:
    '''
    Determine a test result against a reference and thresholds.

    Args:
        image: The image being compressed.
        reference: The reference result to compare against.
        result: The test result.

    Return:
        The result code.
    '''
    psnr_diff = result.psnr - reference.psnr

    if (psnr_diff < RESULT_THRESHOLD_FAIL) and (not image.is_3d):
        return ResultStatus.FAIL

    if (psnr_diff < RESULT_THRESHOLD_3D_FAIL) and image.is_3d:
        return ResultStatus.FAIL

    if psnr_diff < RESULT_THRESHOLD_WARN:
        return ResultStatus.WARN

    return ResultStatus.PASS


def format_solo_result(image: TestImage, result: Record) -> str:
    '''
    Format a metrics string for a single (no compare) result.

    Args:
        image: The image being tested.
        result: The test result.

    Return:
        The metrics report.
    '''
    del image  # Unused

    name = f'{result.block_size:>5} {result.name}'
    psnr = f'{result.psnr:2.3f} dB'
    total_time = f'{result.total_time:.3f} s'
    coding_time = f'{result.coding_time:.3f} s'
    coding_rate = f'{result.coding_rate:.3f} MT/s'

    fields = (
        f'{name:<32}',
        f'{psnr:>11}',
        f'{total_time:>9}',
        f'{coding_time:>9}',
        f'{coding_rate:>11}',
    )

    return ' | '.join(fields)


def format_compare_result(image: TestImage, reference: Record,
                          result: Record) -> str:
    '''
    Format a metrics string for a comparison result.

    Args:
        image: The image being tested.
        reference: The reference result to compare against.
        result: The test result.

    Return:
        The metrics report.
    '''
    psnr_diff = result.psnr - reference.psnr

    try:
        total_time_rel = reference.total_time / result.total_time
    except ZeroDivisionError:
        total_time_rel = float('NaN')

    try:
        coding_time_rel = reference.coding_time / result.coding_time
    except ZeroDivisionError:
        coding_time_rel = float('NaN')

    name = f'{result.block_size:>5} {result.name}'
    psnr = f'{result.psnr:2.3f} dB ({psnr_diff:1.3f} dB)'
    total_time = f'{result.total_time:.3f} s ({total_time_rel:1.2f}x)'
    coding_time = f'{result.coding_time:.3f} s ({coding_time_rel:1.2f}x)'
    coding_rate = f'{result.coding_rate:.3f} MT/s'
    result_status = determine_result(image, reference, result)

    fields = (
        f'{name:<32}',
        f'{psnr:>22}',
        f'{total_time:>15}',
        f'{coding_time:>15}',
        f'{coding_rate:>11}',
        f'{coding_rate:>11}',
        result_status.name
    )

    return ' | '.join(fields)


def run_test_image(encoder: te.EncoderBase, reference: Optional[ResultSet],
                   image: TestImage, quality: str, block_size: str,
                   repeats: int, keep_output: bool,
                   threads: Optional[int]) -> tuple[Record, str]:
    '''
    Execute one test image in the test set.

    Args:
        encoder: The encoder to test.
        reference: The reference test results to compare against.
        image: The single image to test.
        quality: The quality level to test.
        block_size: The block size to test.
        repeats: The number of test repeats to run.
        keep_output: Should the test preserve output images?
        threads (int or None): The thread count to use, or None to use
            automatic thread count based on core count of host machine.

    Return:
        The test results, passed as a test record and a formatted log line
        to emit.
    '''
    res = encoder.run_test(
        image, block_size, f'-{quality}', repeats,
        keep_output, threads)

    record = Record(block_size, image.file_name, *res)

    if reference:
        record_ref = reference.get_matching_record(record)
        record.set_status(determine_result(image, record_ref, record))
        record.set_relative_to_reference(record_ref)

        report = format_compare_result(image, record_ref, record)
    else:
        report = format_solo_result(image, record)

    return (record, report)


def run_test_set(encoder: te.EncoderBase, reference: Optional[ResultSet],
                 test_set: TestSet, quality: str, block_sizes: list[str],
                 repeats: int, keep_output: bool,
                 threads: Optional[int]) -> ResultSet:
    '''
    Execute all tests in the test set.

    Args:
        encoder: The encoder to test.
        reference: The reference test results to compare against.
        test_set: The set of images to test.
        quality: The quality level to test.
        block_sizes: The block sizes to test.
        repeats: The number of test repeats to run.
        keep_output: Should the test preserve output images?
        threads (int or None): The thread count to use, or None to use
            automatic thread count based on core count of host machine.

    Return:
        The test results.
    '''
    result_set = ResultSet(test_set.name)

    current_test_index = 0
    max_test_index = count_test_set(test_set, block_sizes)

    title = f'Test Set: {test_set.name} / Encoder: {encoder.name} -{quality}'
    print(title)
    print('=' * len(title))

    for block_size in block_sizes:
        for image in test_set.tests:
            # 3D block sizes require 3D images
            if is_3d(block_size) != image.is_3d:
                continue

            current_test_index += 1

            progress = f'{current_test_index}/{max_test_index}'
            msg = f'Running {progress} {block_size} {image.file_name} ... '
            print(msg, end='', flush=True)

            record, report = run_test_image(
                encoder, reference, image, quality, block_size,
                repeats, keep_output, threads)

            result_set.add_record(record)

            print(f'\r[{current_test_index:>3}] {report}')

    return result_set


def get_encoder_params(encoder_name: str, reference_name: str,
                       test_set_name: str) \
                       -> tuple[te.EncoderBase, str, str, str]:
    '''
    The the encoder and image set parameters for a test run.

    Args:
        encoder_name: The encoder name.
        reference_name: The reference encoder name.
        test_set_name: The test image set.

    Return:
        The test parameters for the requested encoder and test set. An instance
        of the encoder wrapper class, the output data name, the output result
        directory, and the reference to use.
    '''
    used_reference_name: Optional[str] = None
    encoder: te.EncoderBase

    if encoder_name.startswith('ref'):
        _, version, simd = encoder_name.split('-')

        # Version-specific variants
        compatible_prefixes = ['5.']
        if any(True for x in compatible_prefixes if version.startswith(x)):
            encoder = te.Encoder2xRel(version, simd)
            name = f'reference-{version}-{simd}'
            out_dir = f'Test/Images/{test_set_name}'

            return (encoder, name, out_dir, reference_name)

        # Latest main
        if version == 'main':
            encoder = te.Encoder2x(simd)
            name = f'reference-{version}-{simd}'
            out_dir = f'Test/Images/{test_set_name}'
            return (encoder, name, out_dir, reference_name)

        assert False, f'Encoder {encoder_name} not recognized'

    encoder = te.Encoder2x(encoder_name)
    name = f'develop-{encoder_name}'
    out_dir = f'TestOutput/{test_set_name}'
    used_reference_name = reference_name.replace('ref', 'reference')
    return (encoder, name, out_dir, used_reference_name)


def parse_command_line():
    '''
    Parse the command line.

    Return:
        Namespace: The parsed command line container.
    '''
    parser = argparse.ArgumentParser()

    # All reference encoders
    reference_encoders = [
        'ref-5.0-neon',
        'ref-5.0-sse2', 'ref-5.0-sse4.1', 'ref-5.0-avx2',

        'ref-main-neon', 'ref-main-sve_256', 'ref-main-sve_128',
        'ref-main-sse2', 'ref-main-sse4.1', 'ref-main-avx2'
    ]

    # All test encoders
    test_encoders = [
        'none', 'native', 'universal',
        'neon', 'sve_256', 'sve_128',
        'sse2', 'sse4.1', 'avx2'
    ]

    test_encoders_arm64 = [
        'neon', 'sve_256', 'sve_128'
    ]

    test_encoders_x86 = [
        'sse2', 'sse4.1', 'avx2'
    ]

    encoder_choices = reference_encoders + test_encoders
    encoder_choices.append('all-aarch64')
    encoder_choices.append('all-x86')

    parser.add_argument('--encoder', dest='encoders', default='avx2',
                        choices=encoder_choices, help='test encoder variant')

    parser.add_argument('--reference', default='ref-main-avx2',
                        choices=reference_encoders,
                        help='reference encoder variant')

    profile_choices = ['ldr', 'ldrs', 'hdr', 'all']
    parser.add_argument('--color-profile', dest='profiles', default='all',
                        choices=profile_choices, help='test color profile')

    format_choices = ['l', 'xy', 'rgb', 'rgba', 'all']
    parser.add_argument('--color-format', dest='formats', default='all',
                        choices=format_choices, help='test color format')

    block_size_choices = list(TEST_BLOCK_SIZES) + ['all']
    parser.add_argument('--block-size', dest='block_sizes', action='append',
                        choices=block_size_choices, help='test block size')

    test_dir = Path(__file__).parent / 'Images'

    test_set_choices = [x.name for x in test_dir.iterdir() if x.is_dir()]
    test_set_choices.sort()
    test_set_choices.append('all')
    parser.add_argument('--test-set', dest='test_sets', default='Small',
                        choices=test_set_choices, help='test image set')

    parser.add_argument('--test-image', dest='test_image', default=None,
                        help='select a specific test image from the test set')

    quality_choices = list(TEST_QUALITIES)
    quality_choices.append('all')
    quality_choices.append('all+')
    parser.add_argument('--test-quality', dest='test_qual', default='thorough',
                        choices=quality_choices, help='test quality')

    parser.add_argument('--repeats', dest='repeats', default=1,
                        type=int, help='test repeat count')

    parser.add_argument('--keep-output', dest='keep_output', default=False,
                        action='store_true', help='keep image output')

    parser.add_argument('-j', dest='threads', default=None,
                        type=int, help='encoder thread count')

    args = parser.parse_args()

    # Turn things into canonical format lists
    if args.encoders == 'all-aarch64':
        args.encoders = test_encoders_arm64
    elif args.encoders == 'all-x86':
        args.encoders = test_encoders_x86
    else:
        args.encoders = [args.encoders]

    if args.test_qual == 'all+':
        args.test_qual = TEST_QUALITIES
    elif args.test_qual == 'all':
        args.test_qual = TEST_QUALITIES
        args.test_qual.remove('verythorough')
        args.test_qual.remove('exhaustive')
    else:
        args.test_qual = [args.test_qual]

    if not args.block_sizes or ('all' in args.block_sizes):
        args.block_sizes = TEST_BLOCK_SIZES

    if args.test_sets == 'all':
        args.test_sets = test_set_choices[:-1]
    else:
        args.test_sets = [args.test_sets]

    if args.profiles == 'all':
        args.profiles = profile_choices[:-1]
    else:
        args.profiles = [args.profiles]

    if args.formats == 'all':
        args.formats = format_choices[:-1]
    else:
        args.formats = [args.formats]

    return args


def main() -> int:
    '''
    The main function.

    Return:
        int: The process return code.
    '''
    # Parse command lines
    args = parse_command_line()

    test_set_count = 0
    worst_result = ResultStatus.NOT_RUN

    for quality in args.test_qual:
        for test_set_name in args.test_sets:
            for encoder_name in args.encoders:
                (encoder, name, out_dir, reference_name) = \
                    get_encoder_params(
                        encoder_name, args.reference, test_set_name)

                test_dir = f'Test/Images/{test_set_name}'
                test_csv = f'{out_dir}/astc_{name}_{quality}_results.csv'

                reference = None
                if reference_name:
                    ref_csv = f'astc_{reference_name}_{quality}_results.csv'
                    ref_csv = f'{test_dir}/{ref_csv}'
                    reference = ResultSet(test_set_name)
                    reference.load_from_file(Path(ref_csv))

                test_set_count += 1
                test_set = TestSet(
                    test_set_name, Path(test_dir),
                    args.profiles, args.formats, args.test_image)

                result_set = run_test_set(
                    encoder, reference, test_set, quality,
                    args.block_sizes, args.repeats,
                    args.keep_output, args.threads)

                result_set.save_to_file(Path(test_csv))

                if reference_name:
                    summary = result_set.get_results_summary()
                    this_result = summary.get_worst_result()
                    worst_result = max(this_result, worst_result)
                    print(summary)

        if (test_set_count > 1) and (worst_result != ResultStatus.NOT_RUN):
            print(f'OVERALL STATUS: {worst_result.name}')

    if worst_result == ResultStatus.FAIL:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
