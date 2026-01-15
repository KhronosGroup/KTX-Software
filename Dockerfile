FROM cgr.dev/chainguard/wolfi-base:latest as prod

COPY ./ktx-tools-arm.zip .
COPY ./ktx-tools-amd.zip .