# Testing OSS-Fuzz

ASTC-Encoder has been integrated into the Google OSS-Fuzz program, which
performs API fuzz testing on a Google-hosted CI infrastructure.

This page is a set of summary instructions explaining how to locally reproduce
failures reported by OSS-Fuzz on a Linux machine. Full documentation is
provided by the OSS-Fuzz project [documentation pages][1].

[1]: https://google.github.io/oss-fuzz/

## Prerequisites

Install Docker:

```sh
sudo apt  install docker.io
sudo usermod -aG docker $USER
```

You will need to log out and log in again for the group changes to take effect.

# Running Python-based CLI functional tests

Check out the OSS-Fuzz project:

```sh
git clone --depth=1 https://github.com/google/oss-fuzz.git
cd oss-fuzz
```

Download the standard Docker images with the tools pre-integrated:

```sh
python3 infra/helper.py pull_images
```

Build the Docker image and the fuzzers for astcenc.

> [!NOTE]
> Fuzzers are built for a specific sanitizer, so you will need to build and run
> the fuzzers multiple times if you want coverage of both ASAN and UBSAN.

```sh
python3 infra/helper.py build_image astc-encoder

# Build using clean checkout in the container
python3 infra/helper.py build_fuzzers astc-encoder

# Build using local checkout mounted into the container
python3 infra/helper.py build_fuzzers astc-encoder /mnt/c/work/projects/astcenc/Source --sanitizer <address,undefined, etc>
```

Run a reproducer test case downloaded from OSS Fuzz:

```sh
python3 infra/helper.py reproduce astc-encoder <fuzz_target> <testcase>
```

Sometimes reproducers are intermittent and do not always reproduce. Running the
test scenario in a loop can be a useful way to try and make it reproduce.

```sh
while python3 infra/helper.py reproduce astc-encoder <fuzz_target> <testcase>; do :; done
```

- - -

_Copyright © 2026, Arm Limited and contributors._
