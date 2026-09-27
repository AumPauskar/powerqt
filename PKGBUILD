pkgname=powerqt
pkgver=0.1.0
pkgrel=1
pkgdesc="Qt battery charge threshold manager"
arch=('x86_64')
license=('MIT')

depends=('qt6-base')
makedepends=('cmake')

build() {
    cmake -B build -S "$startdir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr

    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
