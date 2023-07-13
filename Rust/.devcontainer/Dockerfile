FROM mcr.microsoft.com/azurespheresdk:latest

USER root
RUN apt update && \
    apt install llvm libc6-dev-i386 libclang-dev patchelf -y

USER vscode
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y && \
    /home/vscode/.cargo/bin/rustup target add armv7-unknown-linux-musleabihf
ENV PATH="/home/vscode/.cargo/bin:${PATH}"
