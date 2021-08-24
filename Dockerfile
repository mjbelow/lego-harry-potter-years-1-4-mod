FROM mstorsjo/llvm-mingw:20210816

WORKDIR /build

COPY . .

RUN mkdir -p /output

CMD ["make", "install"]
