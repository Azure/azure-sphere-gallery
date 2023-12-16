FROM mcr.microsoft.com/azurespheresdk:latest

RUN apt update && \
    apt install -y libncurses5 gdb && \
    apt autoremove -y && \
    apt clean -y
