## Simple KV storage

I'm also testing the usability of custom struct error-handling as a function return value.

### Strengths
- Centralized Error Reporting: Using a custom structure (ErrBox) to encapsulate error information makes error handling more consistent and easier to manage.
- Flexibility: The ErrBox structure allows attaching arbitrary data (void *ret_value) therefore it works for every return type. This also can lead to some problems as the caller needs to know what return value they expect and cast it to its proper type.
- Clear Error Indication: The RETURN_TYPE enumeration can clearly indicate whether an operation was successful or resulted in an error, which makes it easy and readible.

### Potential Weaknesses
- Error Propagation: Returning an ErrBox pointer from functions means that error handling logic must be distributed across all parts of program that call these functions. This can lead to scattered error handling code, which might be harder to maintain and understand. (Developing some custom struct/functions for error handling might solve this problem)
- Potential for Confusion: Using a single structure (ErrBox) for both success and error cases can potentially lead to confusion, especially if the structure's fields are reused or if there's a lack of clear separation between success and error handling paths in the code.

Also sometimes I was split whether I should use RETURN_TYPE_SUCCESS or RETURN_TYPE_ERROR when the operation was unsuccessfull but this wouldn't lead to crash.

#### Example:
```
ErrBox *action_get(const char *key) {
  ErrBox *search = find_key(key);

  if (search->ret_outcome == RETURN_TYPE_SUCCESS) {
    if (search->ret_value != NULL) {
      fprintf(stdout, "%s,%s", ((KVBox *)search->ret_value)->key,
              ((KVBox *)search->ret_value)->value);
    } else {
      free(search);
      fprintf(stdout, "%s not found\n", key);
      return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
    }
  }
  return search;
}
```
#### Success: 
Function returns RETURN_TYPE_SUCCESS with key if it was found.

#### Fail:
I was split between using RETURN_TYPE_ERROR with err_desc="The key was not found" and RETURN_TYPE_SUCCESS but printing error from inside this function.

Still don't know which option is better, propably it doesn't matter and it's just me overcomplicating things.



Next is [Evan Teran's method](https://stackoverflow.com/questions/291828/what-is-the-best-way-to-return-an-error-from-a-function-when-im-already-returni#291830) also could be called someone else's method, but I found out about it from his post.
