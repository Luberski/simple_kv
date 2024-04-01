#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

typedef enum {
  RETURN_TYPE_SUCCESS,
  RETURN_TYPE_ERROR,
} RETURN_TYPE;

typedef struct {
  RETURN_TYPE ret_outcome;
  void *ret_value;
  char *err_desc;
} ErrBox;

typedef struct {
  char initializer_var;
  char action;
  char *key;
  char *value;
} Command;

ErrBox *ErrBox_init_null() {
  ErrBox *ret = malloc(sizeof(ErrBox));

  ret->ret_outcome = RETURN_TYPE_ERROR;
  ret->ret_value = NULL;
  ret->err_desc = NULL;

  return ret;
}

ErrBox *ErrBox_init(RETURN_TYPE outcome, void *value, char *description) {
  ErrBox *ret = malloc(sizeof(ErrBox));

  ret->ret_outcome = outcome;
  ret->ret_value = value;
  ret->err_desc = description;

  return ret;
}

Command *Command_init(char action, char *key, char *value) {
  Command *ret = malloc(sizeof(Command));
  ret->action = action;
  ret->key = key;
  ret->value = value;
  ret->initializer_var = 3;

  return ret;
}

Command *Command_init_null() {
  Command *ret = malloc(sizeof(Command));
  ret->action = 0;
  ret->key = NULL;
  ret->value = NULL;
  ret->initializer_var = 0;

  return ret;
}

ErrBox *Command_part_init(Command *self, char *part) {
  ErrBox *ret = NULL;

  switch (self->initializer_var) {
  case 0:
    if (strlen(part) == 1) {
      self->action = *part;
    } else {
      ret = ErrBox_init(
          RETURN_TYPE_ERROR, NULL,
          "error: action part should be exactly 1 character long\n");
    }
    ret = ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
    break;

  case 1:
    self->key = malloc((strlen(part) + 1) * sizeof(char));
    strcpy(self->key, part);
    ret = ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
    break;

  case 2:
    self->value = malloc((strlen(part) + 1) * sizeof(char));
    strcpy(self->value, part);
    printf("str: %s, len: %lu\n", self->value, strlen(part));
    ret = ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
    break;

  default:
    return ErrBox_init(RETURN_TYPE_ERROR, NULL,
                       "error: Command is already initialized\n");
  }

  self->initializer_var++;
  return ret;
}

void print_usage() {
  fprintf(stdout, "usage: kv <option>,<key>,<value> ...\n");
}

// AKSHUALLY duo commas + \0
ErrBox *tres_commas_split(const char *strbuf) {
  unsigned char comma_count = 0;
  Command *cmd = Command_init_null();
  unsigned char point_after_last_comma = 0;

  for (int str_iter = 0; str_iter <= strlen(strbuf); str_iter++) {
    if ((*(strbuf + str_iter) == ',' || *(strbuf + str_iter) == '\0') &&
        comma_count < 3) {
      // distance from first letter to end + nullbyte
      char string_split_buffer[str_iter - point_after_last_comma + 1];
      // zero out memory because of skill issues
      memset(string_split_buffer, '\0', sizeof(string_split_buffer));
      // src, argument+offset, distance
      strncpy(string_split_buffer, strbuf + point_after_last_comma,
              (str_iter - point_after_last_comma));

      ErrBox *output = Command_part_init(cmd, string_split_buffer);
      if (output->ret_outcome == RETURN_TYPE_ERROR) {
        return output;
      }

      comma_count++;
      point_after_last_comma = str_iter + 1;
    }
  }
  if (comma_count != 3) {
    ErrBox *ret =
        ErrBox_init(RETURN_TYPE_ERROR, NULL,
                    "error: invalid operation format (comma_count != 2)\n");
    return ret;
  }

  return ErrBox_init(RETURN_TYPE_SUCCESS, cmd, NULL);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    print_usage();
  }

  for (size_t arg_iter = 1; arg_iter < argc; arg_iter++) {
    ErrBox *lol = tres_commas_split(argv[arg_iter]);
    if (lol->ret_outcome == RETURN_TYPE_SUCCESS) {
      Command *lolol = (Command *)lol->ret_value;
      printf("%c, %s, %s\n", lolol->action, lolol->key, lolol->value);
      free(lolol->value);
      free(lolol->key);
    } else {
      printf("%s\n", lol->err_desc);
    }

    free(lol->err_desc);
    free(lol->ret_value);
    free(lol);
  }
  return 0;
}
