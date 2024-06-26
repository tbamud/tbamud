# unit and integration tests for tbamud

## how do I add a new test?
Open the .c file of your choosing and add a `UNIT_TEST` function. 
The function will have access to all the global variables and all non-static 
functions in the code, but there will be no data loaded.

The name of the function will be listed when the tests are run.




The [munit website](https://nemequ.github.io/munit/#getting-started) may be useful for more details. 


## how do I add new files with tests?
First, create your test file. As a general rule, keep unit tests in files named
  after the files containing the functions you are testing. For instance, if you're
  testing the `do_simple_move()` function, create a file called `test.act.movement.c`.
  
You can use the example file `test.example.c` as a template. 
The `.c`-file needs a couple of boilerplate parts:

- An import statement to include the `.h`-file.
- UNIT_TEST-functions. See above.
- An array of `MunitTest`s for inclusion in the runner app. 
  The name in these are concatenated between the name in testrunner and the name of the tests in the output.
  This is useful for grouping.

Next, create a header file for your tests. It's a good idea to keep the same name,
  with a .h postfix. So in the example, it'll be `test.act.movement.h`.
  
You can use the `test.example.h` file as a template. It needs a little boilerplate, too.

- It needs to include the testrunner.h for the prototype of UNIT_TEST and access to munit-structs.
- It needs a guard to only be loaded once (the `#ifndef/#define/#endif` incantation at the start and end)
- It needs a prototype of all tests in the .c-file.
- It needs a prototype of the array of tests.

Finally, add the array to the suites array in `testrunner.c` to actually run the tests.

- Add the .h file to the list of imported files.
- Add a row to the suites array. The name in this list is prepended to every test in the given 
  file when listing the results.

Now, having all the bits and pieces ready, you can add you unit tests, and run them with `make test`

