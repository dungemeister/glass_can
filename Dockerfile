FROM gcc:13-bookworm

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libssl-dev \
    libsqlite3-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

 WORKDIR /app

 COPY . .

 RUN rm -rf build && mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_RPATH="/app/third_party/curlpp" \
        -DCMAKE_INSTALL_RPATH="/app/third_party/curlpp" \
        -DCMAKE_CXX_STANDARD=17 .. && \
    make -j$(nproc)

RUN cd third_party/curlpp && \
    ln -sf libcurlpp.so.1.0.0 libcurlpp.so.1 && \
    ln -sf libcurlpp.so.1.0.0 libcurlpp.so
#  COPY --from=builder /app/build/your_app /usr/local/bin/
 CMD ["/app/build/glass_can"]


