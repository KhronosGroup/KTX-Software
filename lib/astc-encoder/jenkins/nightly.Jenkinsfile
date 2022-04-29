/* This pipeline is used for post-commit testing, so it runs frequently.
 *
 * Test objectives for this pipeline are:
 *
 *   - Run the entire pipeline in less than 10 minutes.
 *   - Test builds on all supported operating systems.
 *   - Test builds on all supported compilers.
 *   - Test release and debug build variants.
 *   - Run functional smoke tests.
 *   - Run image quality smoke tests.
 *
 * The test matrix is not fully covered; e.g. we can assume compilers behave
 * similarly on different operating systems, so we test one compiler per OS.
 */

@Library('hive-infra-library@changes/86/295486/1') _

pipeline {
  agent none

  options {
    ansiColor('xterm')
    timestamps()
  }

  stages {
    stage('Build All') {
      parallel {
        /* Build for Linux on x86-64 using GCC */
        stage('Linux') {
          agent {
            kubernetes {
              yaml '''\
                apiVersion: v1
                kind: Pod
                spec:
                  securityContext:
                    runAsUser: 1000
                    runAsGroup: 1000
                  imagePullSecrets:
                    - name: artifactory-ms-docker
                  containers:
                    - name: astcenc
                      image: mobile-studio--docker.eu-west-1.artifactory.aws.arm.com/astcenc:3.1.0
                      command:
                        - sleep
                      args:
                        - infinity
                      resources:
                        requests:
                          cpu: 4
                          memory: 8Gi
                        limits:
                          cpu: 8
                          memory: 16Gi
              '''.stripIndent()
              defaultContainer 'astcenc'
            }
          }
          stages {
            stage('Clean') {
              steps {
                sh '''
                    git clean -ffdx
                    git submodule init
                    git submodule update
                '''
              }
            }
            stage('Build R') {
              steps {
                sh '''
                  mkdir build_rel
                  cd build_rel
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DISA_NONE=ON -DUNITTEST=ON -DPACKAGE=x64 ..
                  make install package -j4
                '''
              }
            }
            stage('Build D') {
              steps {
                sh '''
                  mkdir build_dbg
                  cd build_dbg
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DISA_NONE=ON ..
                  make -j4
                '''
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-linux-x64', includes: '*.zip'
                }
              }
            }
            stage('Test') {
              steps {
                sh '''
                  python3 ./Test/astc_test_functional.py --encoder=none
                  python3 ./Test/astc_test_functional.py --encoder=sse2
                  python3 ./Test/astc_test_functional.py --encoder=sse4.1
                  python3 ./Test/astc_test_functional.py --encoder=avx2
                  python3 ./Test/astc_test_image.py --encoder=all-x86 --test-set Small --test-quality medium
                '''
                dir('build_rel') {
                  sh 'ctest'
                }
              }
            }
          }
        }
        /* Build for Windows on x86-64 using MSVC */
        stage('Windows MSVC') {
          agent {
            label 'Windows'
          }
          stages {
            stage('Clean') {
              steps {
                bat 'git clean -ffdx'
              }
            }
            stage('Build R') {
              steps {
                bat '''
                  call c:\\progra~2\\micros~1\\2019\\buildtools\\vc\\auxiliary\\build\\vcvars64.bat
                  mkdir build_rel
                  cd build_rel
                  cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DPACKAGE=x64-cl ..
                  nmake install package
                '''
              }
            }
            stage('Build D') {
              steps {
                bat '''
                  call c:\\progra~2\\micros~1\\2019\\buildtools\\vc\\auxiliary\\build\\vcvars64.bat
                  mkdir build_dbg
                  cd build_dbg
                  cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DISA_NONE=ON ..
                  nmake
                '''
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-windows-x64-cl', includes: '*.zip'
                }
              }
            }
            stage('Test') {
              steps {
                bat '''
                  set Path=c:\\Python38;c:\\Python38\\Scripts;%Path%
                  call python ./Test/astc_test_image.py --test-set Small --test-quality medium
                '''
              }
            }
          }
        }
        /* Build for Windows on x86-64 using MSVC + ClangCL */
        stage('Windows ClangCL') {
          agent {
            label 'Windows'
          }
          stages {
            stage('Clean') {
              steps {
                bat 'git clean -ffdx'
              }
            }
            stage('Build R') {
              steps {
                bat '''
                  call c:\\progra~2\\micros~1\\2019\\buildtools\\vc\\auxiliary\\build\\vcvars64.bat
                  mkdir build_rel
                  cd build_rel
                  cmake -G "Visual Studio 16 2019" -T ClangCL -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DPACKAGE=x64-clangcl ..
                  msbuild astcencoder.sln -property:Configuration=Release
                  msbuild PACKAGE.vcxproj -property:Configuration=Release
                  msbuild INSTALL.vcxproj -property:Configuration=Release
                '''
              }
            }
            stage('Build D') {
              steps {
                bat '''
                  call c:\\progra~2\\micros~1\\2019\\buildtools\\vc\\auxiliary\\build\\vcvars64.bat
                  mkdir build_dbg
                  cd build_dbg
                  cmake -G "Visual Studio 16 2019" -T ClangCL -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON ..
                  msbuild astcencoder.sln -property:Configuration=Debug
                '''
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-windows-x64-clangcl', includes: '*.zip'
                }
              }
            }
            stage('Test') {
              steps {
                bat '''
                  set Path=c:\\Python38;c:\\Python38\\Scripts;%Path%
                  call python ./Test/astc_test_image.py --test-set Small --test-quality medium
                '''
              }
            }
          }
        }
        /* Build for macOS on x86-64 using Clang */
        stage('macOS') {
          agent {
            label 'mac'
          }
          stages {
            stage('Clean') {
              steps {
                sh 'git clean -ffdx'
              }
            }
            stage('Build R') {
              steps {
                sh '''
                  mkdir build_rel
                  cd build_rel
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DPACKAGE=x64 ..
                  make install package -j4
                '''
              }
            }
            stage('Build D') {
              steps {
                sh '''
                  mkdir build_dbg
                  cd build_dbg
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DISA_NONE=ON ..
                  make -j4
                '''
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-macos-x64', includes: '*.zip'
                }
              }
            }
            stage('Test') {
              steps {
                sh '''
                  export PATH=/usr/local/bin:$PATH
                  python3 ./Test/astc_test_image.py --test-set Small --test-quality medium
                '''
              }
            }
          }
        }
      }
    }
    stage('Artifactory') {
      agent {
        kubernetes {
          yaml '''
apiVersion: v1
kind: Pod
spec:
  securityContext:
    runAsUser: 1000
    runAsGroup: 1000
  imagePullSecrets:
    - name: artifactory-ms-docker
  containers:
    - name: astcenc
      image: mobile-studio--docker.eu-west-1.artifactory.aws.arm.com/astcenc:3.0.0
      command:
        - sleep
      args:
        - infinity
      resources:
        requests:
          cpu: 1
          memory: 4Gi
'''
          defaultContainer 'astcenc'
        }
      }
      options {
        skipDefaultCheckout true
      }
      stages {
        stage('Unstash') {
          steps {
            dir('upload/linux-x64') {
              unstash 'astcenc-linux-x64'
            }
            dir('upload/windows-x64-cl') {
              unstash 'astcenc-windows-x64-cl'
            }
            dir('upload/windows-x64-clangcl') {
              unstash 'astcenc-windows-x64-clangcl'
            }
            dir('upload/macos-x64') {
              unstash 'astcenc-macos-x64'
            }
          }
        }
        stage('Upload') {
          steps {
            zip zipFile: 'astcenc.zip', dir: 'upload', archive: false
            cepeArtifactoryUpload(sourcePattern: 'astcenc.zip')
          }
        }
      }
      post {
        always {
          deleteDir()
        }
      }
    }
  }

  post {
    failure {
      script {
        slackSend channel: '#dsg-eng-astcenc', color: 'danger', message: "Build ${JOB_NAME} ${BUILD_NUMBER} failed. (<${BUILD_URL}|Open>)", teamDomain: 'arm-dsg', tokenCredentialId: 'jenkins-slack', username: 'jenkins'
      }
    }
  }
}
