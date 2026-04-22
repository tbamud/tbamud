Files for tbaMUD.

## Unit Tests

tbaMUD ships with a C unit-test suite located in the `tests/` directory.
The suite uses the [Unity](https://github.com/ThrowTheSwitch/Unity) test
framework (vendored under `tests/vendor/unity/`).

### Quick start

```
./configure
cd tests && make test
```

`make test` builds each test binary, runs it, and writes JUnit XML results to
`tests/test-results/`.  A summary is printed to the terminal:

```
[PASS] test_utils
[PASS] test_random
[PASS] test_interpreter
[PASS] test_class
```

### CI

The GitHub Actions workflow (`.github/workflows/build.yml`) runs `make test`
on every push and pull request against `master` and publishes a formatted
report via the `dorny/test-reporter` action.

See [doc/testing.md](doc/testing.md) for full details on adding new tests and
understanding the test infrastructure.

