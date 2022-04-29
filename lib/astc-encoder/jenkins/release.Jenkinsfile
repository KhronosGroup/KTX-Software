/* This pipeline is used for release testing, so it runs rarely.
 *
 * Test objectives for this pipeline are:
 *
 *   - Run the entire pipeline in less than 60 minutes.
 *   - Test builds on all supported operating systems.
 *   - Test builds on optimized compiler choices (i.e. prefer Clang over GCC).
 *   - Build only release variants.
 *   - Run full functional tests.
 *   - Run full image quality tests.
 *   - Code sign the binaries on supported operating systems.
 *   - Build the release package.
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
        /* Run static analysis on Linux */
        stage('Coverity') {
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
      image: mobile-studio--docker.eu-west-1.artifactory.aws.arm.com/astcenc:3.1.0
      command:
        - sleep
      args:
        - infinity
      resources:
        requests:
          cpu: 4
          memory: 8Gi
'''
            defaultContainer 'astcenc'
            }
          }
          stages {
            stage('Clean') {
              steps {
                sh 'git clean -fdx'
              }
            }
            stage('Coverity') {
              steps {
                withCredentials([usernamePassword(credentialsId: 'jenkins-password',
                                                  usernameVariable: 'USERNAME',
                                                  passwordVariable: 'PASSWORD')]) {
                  sh script: '''#!/bin/bash
                    mkdir -p ${WORKSPACE}/occonfig

                    mkdir build_cov
                    cd build_cov

                    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON ..

                    cov-configure --config ${WORKSPACE}/coverity.conf --template --compiler cc --comptype gcc
                    cov-configure --config ${WORKSPACE}/coverity.conf --template --compiler c++ --comptype g++
                    cov-build --config ${WORKSPACE}/coverity.conf --dir ${WORKSPACE}/intermediate make install
                    cov-analyze --dir ${WORKSPACE}/intermediate
                    cov-commit-defects --dir ${WORKSPACE}/intermediate \\
                                       --stream astcenc-master \\
                                       --url https://coverity.cambridge.arm.com \\
                                       --user jenkins@arm.com --password ${PASSWORD} \\
                                       --strip-path ${WORKSPACE}
                  '''
                }
              }
            }
          }
        }
        /* Build for Linux on x86-64 using Clang */
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
                      image: mobile-studio--docker.eu-west-1.artifactory.aws.arm.com/astcenc:3.0.0
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
                sh 'git clean -ffdx'
              }
            }
            stage('Build astcenc R') {
              steps {
                sh '''
                  export CXX=clang++-9
                  mkdir build_rel
                  cd build_rel
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DISA_NONE=ON -DPACKAGE=x64 ..
                  make install package -j4
                '''
              }
            }
            stage('Build astcdec R') {
              steps {
                sh '''
                  export CXX=clang++-9
                  mkdir build_reldec
                  cd build_reldec
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DISA_NONE=ON -DDECOMPRESSOR=ON ..
                  make -j4
                '''
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-linux-x64', includes: '*.zip'
                  stash name: 'astcenc-linux-x64-hash', includes: '*.zip.sha256'
                }
              }
            }
            stage('Test') {
              steps {
                sh '''
                  python3 ./Test/astc_test_functional.py
                  python3 ./Test/astc_test_image.py --encoder=all-x86 --test-set Small
                '''
              }
            }
          }
        }
        /* Build for Windows on x86-64 using MSVC ClangCL */
        stage('Windows') {
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
                  cmake -G "Visual Studio 16 2019" -T ClangCL -DCMAKE_INSTALL_PREFIX=../ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON -DPACKAGE=x64 ..
                  msbuild astcencoder.sln -property:Configuration=Release
                  msbuild PACKAGE.vcxproj -property:Configuration=Release
                  msbuild INSTALL.vcxproj -property:Configuration=Release
                '''
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-windows-x64', includes: '*.zip'
                  stash name: 'astcenc-windows-x64-hash', includes: '*.zip.sha256'
                }
              }
            }
            stage('Test') {
              steps {
                bat '''
                  set Path=c:\\Python38;c:\\Python38\\Scripts;%Path%
                  call python ./Test/astc_test_image.py --test-set Small
                '''
              }
            }
          }
        }
        /* Build for macOS on x86-64 using Clang */
        stage('macOS') {
          agent {
            label 'mac && notarizer'
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
            stage('Sign and notarize') {
              environment {
                NOTARIZATION_CREDS = credentials('notarization-account')
              }
              steps {
                dir('build_rel') {
                  sh 'git clone ssh://eu-gerrit-1.euhpc.arm.com:29418/Hive/shared/signing'
                  withCredentials([usernamePassword(credentialsId: 'win-signing',
                                                    usernameVariable: 'USERNAME',
                                                    passwordVariable: 'PASSWORD')]) {
                    sh 'python3 ./signing/macos-client-wrapper.py ${USERNAME} *.zip'
                    sh 'rm -rf ./signing'
                  }
                }
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-macos-x64', includes: '*.zip'
                  stash name: 'astcenc-macos-x64-hash', includes: '*.zip.sha256'
                }
              }
            }
            stage('Test') {
              steps {
                sh '''
                  export PATH=/usr/local/bin:$PATH
                  python3 ./Test/astc_test_image.py --test-set Small
                '''
              }
            }
          }
        }
        /* Build for macOS on x86-64 using Clang */
        stage('macOS arm64') {
          agent {
            label 'mac && notarizer'
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
                  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DISA_NEON=ON -DPACKAGE=aarch64 ..
                  make install package -j4
                '''
              }
            }
            stage('Sign and notarize') {
              environment {
                NOTARIZATION_CREDS = credentials('notarization-account')
              }
              steps {
                dir('build_rel') {
                  sh 'git clone ssh://eu-gerrit-1.euhpc.arm.com:29418/Hive/shared/signing'
                  withCredentials([usernamePassword(credentialsId: 'win-signing',
                                                    usernameVariable: 'USERNAME',
                                                    passwordVariable: 'PASSWORD')]) {
                    sh 'python3 ./signing/macos-client-wrapper.py ${USERNAME} *.zip'
                    sh 'rm -rf ./signing'
                  }
                }
              }
            }
            stage('Stash') {
              steps {
                dir('build_rel') {
                  stash name: 'astcenc-macos-aarch64', includes: '*.zip'
                  stash name: 'astcenc-macos-aarch64-hash', includes: '*.zip.sha256'
                }
              }
            }
            // TODO: Currently can't test automatically
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
            dir('upload') {
              unstash 'astcenc-linux-x64-hash'
              unstash 'astcenc-macos-x64-hash'
              unstash 'astcenc-macos-aarch64-hash'

              unstash 'astcenc-linux-x64'
              unstash 'astcenc-macos-x64'
              unstash 'astcenc-macos-aarch64'
            }
            dir('upload/windows-x64') {
              unstash 'astcenc-windows-x64'
              dir('signing') {
                checkout changelog: false,
                         poll: false,
                         scm: [$class: 'GitSCM',
                               branches: [[name: '*/master']],
                               doGenerateSubmoduleConfigurations: false,
                               extensions: [],
                               submoduleCfg: [],
                               userRemoteConfigs: [[credentialsId: 'gerrit-jenkins-ssh',
                                                    url: 'ssh://mirror.eu-west-1.gerrit-eu01.aws.arm.com:29418/Hive/shared/signing']]]
              }
              withCredentials([usernamePassword(credentialsId: 'win-signing',
                                                usernameVariable: 'USERNAME',
                                                passwordVariable: 'PASSWORD')]) {
                sh 'python3 ./signing/windows-client-wrapper.py ${USERNAME} *.zip'
                sh 'mv *.zip.sha256 ../'
                sh 'mv *.zip ../'
              }
            }
            dir('upload') {
              sh 'rm -rf ./windows-x64'
              sh 'cat *.sha256 > release-sha256.txt'
              sh 'rm *.sha256'
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
