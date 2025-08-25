FROM archlinux:latest

RUN pacman -Sy --noconfirm postgresql-libs libpqxx cmake gcc make pkgconf

WORKDIR /app
COPY . .

RUN cmake -DCMAKE_BUILD_TYPE=Release . && \
    make -j$(nproc)

CMD ["./replicator"]
