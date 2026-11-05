# How to Create a Release

This guide explains how to build the `.deb` installer package and package the source archive for GitHub Releases.

---

## 1. Prerequisites

Ensure required build tools and libraries are installed (Debian-based):
```bash
sudo apt update
sudo apt install -y cmake g++ libqt6widgets6 libqt6network6 qt6-base-dev libhidapi-dev rpm
```

---

## 2. Build Release Binary

Configure and compile with `Release` optimizations:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Run framing self-checks to ensure zero protocol regressions:
```bash
./build/test-framing
```

---

## 3. Generate Packages (`.deb` & `.rpm`)

Generate packages using CPack (supports both DEB and RPM if `rpm` is installed):
```bash
cd build
cpack -G "DEB;RPM"
cd ..
```

The generated files will be located at:
```text
build/thor-404-controller-1.0.0-Linux.deb
build/thor-404-controller-1.0.0-Linux.rpm
```

To test local Debian installation:
```bash
sudo apt install ./build/thor-404-controller-1.0.0-Linux.deb
```

---

## 4. Create Source Code Archive

Create a clean `.zip` of the current Git tree:
```bash
git archive --format=zip --prefix=thor-404-controller-v1.0.0/ HEAD -o build/thor-404-controller-v1.0.0-source.zip
```

---

## 5. Publish to GitHub Releases

1. Go to your repository on GitHub $\rightarrow$ **Releases** $\rightarrow$ **Draft a new release**.
2. Create a new tag: e.g. `v1.0.0`.
3. Set the Release title: e.g. `Genesis Thor 404 Controller v1.0.0`.
4. Drag and attach the generated release artifacts from `build/`:
   - `build/thor-404-controller-1.0.0-Linux.deb`
   - `build/thor-404-controller-v1.0.0-source.zip`
5. Click **Publish release**.
