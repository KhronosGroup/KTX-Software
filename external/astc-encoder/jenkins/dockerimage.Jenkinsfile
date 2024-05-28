pipeline {
  agent {
    kubernetes {
      yaml '''
apiVersion: v1
kind: Pod
spec:
  imagePullSecrets:
    - name: artifactory-ms-docker
  containers:
    - name: dind
      image: mobile-studio--docker.eu-west-1.artifactory.aws.arm.com/docker:dind
      tty: true
      resources:
        requests:
          cpu: 2
          memory: 4Gi
      securityContext:
        privileged: true
      volumeMounts:
        - name: dind-storage
          mountPath: /var/lib/docker
  volumes:
    - name: dind-storage
      emptyDir: {}
'''
      defaultContainer 'dind'
    }
  }
  environment {
    ARTIFACTORY_CREDENTIALS = credentials('cepe-artifactory-jenkins')
  }
  options {
    ansiColor('xterm')
    timestamps()
  }
  stages {
    stage('Build and Push Image') {
      steps {
        sh 'docker info'
        sh '''
          apk add --no-cache bash curl
          chmod u+x ./jenkins/build-image.sh
          ./jenkins/build-image.sh push
        '''
      }
    }
  }
}
