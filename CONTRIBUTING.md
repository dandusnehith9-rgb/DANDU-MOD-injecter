# Contributing to DANDU MOD injecter

Thanks for helping improve the project.

## Development Setup

1. Install Qt (Widgets), CMake, and a C++17 compiler.
2. Clone the repository.
3. Configure and build:

```powershell
cmake -S . -B build-qt -G Ninja `
  -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/Ninja/ninja.exe" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/mingw_64" `
  -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build-qt
```

## Branch Naming

- `feature/<short-description>`
- `fix/<short-description>`
- `docs/<short-description>`

## Pull Request Checklist

- Build succeeds locally.
- Change is scoped and focused.
- README/docs updated if behavior changed.
- UI changes include a screenshot or short demo note.
- Commit messages explain intent clearly.

## Code Style

- C++17
- Keep methods small and readable.
- Prefer clear names over clever code.
- Add comments only where logic is non-obvious.

## Bug Reports

Please include:

- OS version
- Qt version
- Steps to reproduce
- Expected result
- Actual result
- Logs/screenshots if available
