#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

const char *KV_DATABASE_PATH = "kv_database.txt";

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
  char **args;
  unsigned char arg_count;
} Arguments;

typedef struct {
  char *key;
  char *value;
} KVBox;

Arguments *Arguments_init(char **args, unsigned char arg_count) {
  Arguments *self = malloc(sizeof(Arguments));
  self->args = args;
  self->arg_count = arg_count;

  return self;
}

void ErrBox_free(ErrBox *self) {
  free(self->ret_value);
  free(self->err_desc);
  free(self);
}

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

ErrBox *KVBox_init_from_data(const char *data) {
  KVBox *output = malloc(sizeof(KVBox));
  if (output == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "malloc failed\n");
  }
  output->key = NULL;
  output->value = NULL;

  size_t key_size = 0;
  for (size_t data_iter = 0; data_iter < strlen(data); data_iter++) {
    if (data[data_iter] == ',') {
      output->key = malloc(data_iter * sizeof(char));
      output->value = malloc(((strlen(data) - data_iter) + 1) * sizeof(char));

      memset(output->key, '\0', data_iter);
      memset(output->value, '\0', ((strlen(data) - data_iter) + 1));

      strncpy(output->key, data, data_iter - 1);
      strcpy(output->value, data + (data_iter + 1));
    }
  }

  if (output->key == NULL || output->value == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL,
                       "Failed initializing kv data\n");
  }
  return ErrBox_init(RETURN_TYPE_SUCCESS, output, NULL);
}

ErrBox *KVBox_init(char *key, char *value) {
  KVBox *output = malloc(sizeof(KVBox));
  if (output == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "malloc failed\n");
  }
  output->key = key;
  output->value = value;

  return ErrBox_init(RETURN_TYPE_SUCCESS, output, NULL);
}

void print_usage() {
  fprintf(stdout, "usage: kv <option>,<key>,<value> ...\n");
}

// AKSHUALLY duo commas + \0
ErrBox *tres_commas_split(const char *strbuf) {
  unsigned char comma_count = 0;
  unsigned char point_after_last_comma = 0;
  unsigned char curr_arg_count = 0;
  unsigned char total_arg_count = 0;

  switch (strbuf[0]) {
  case 'p':
    total_arg_count = 3;
    break;
  case 'g':
    total_arg_count = 2;
    break;
  case 'd':
    total_arg_count = 2;
    break;
  case 'c':
    total_arg_count = 1;
    break;
  case 'a':
    total_arg_count = 1;
    break;
  default:
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "Invalid argument\n");
  }

  char **args = malloc(total_arg_count * sizeof(char *));
  for (int str_iter = 0; str_iter <= strlen(strbuf); str_iter++) {
    if ((*(strbuf + str_iter) == ',' || *(strbuf + str_iter) == '\0')) {
      curr_arg_count++;
      if (((str_iter - point_after_last_comma) > 1) && curr_arg_count == 1) {
        free(args);
        return ErrBox_init(RETURN_TYPE_ERROR, NULL,
                           "First argument should be 1 letter\n");
      }
      if (curr_arg_count > total_arg_count) {
        for (int args_count = 0; args_count < total_arg_count; args_count++) {
          free(args[args_count]);
        }
        free(args);
        return ErrBox_init(RETURN_TYPE_ERROR, NULL,
                           "Too many arguments provided\n");
      }

      // distance from first letter to end + nullbyte
      args[curr_arg_count - 1] =
          malloc((str_iter - point_after_last_comma + 1) * sizeof(char));
      // zero out memory because of skill issues
      memset(args[curr_arg_count - 1], '\0',
             strlen(args[curr_arg_count - 1]) + 1);
      // src, argument+offset, distance
      strncpy(args[curr_arg_count - 1], strbuf + point_after_last_comma,
              (str_iter - point_after_last_comma));

      comma_count++;
      point_after_last_comma = str_iter + 1;
    }
  }
  Arguments *output = Arguments_init(args, total_arg_count);
  return ErrBox_init(RETURN_TYPE_SUCCESS, output, NULL);
}

ErrBox *find_key(const char *key) {
  FILE *database = fopen(KV_DATABASE_PATH, "r");
  if (database == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "fopen failed\n");
  }

  size_t line_size = 0;
  char *line = NULL;
  while (getline(&line, &line_size, database)) {
    if (line[0] == '\0') {
      break;
    }

    printf("line: %s\n", line);
    ErrBox *data = KVBox_init_from_data(line);
    if (data->ret_outcome == RETURN_TYPE_SUCCESS) {
      KVBox *kv_data = data->ret_value;
      if (!strcmp(kv_data->key, key)) {
        fclose(database);
        free(line);
        return data;
      } else {
        free(kv_data->value);
        free(kv_data->key);
        free(kv_data);
      }
    } else {
      fclose(database);
      return data;
    }
  }

  fclose(database);
  return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
}

ErrBox *action_put(const char *key, const char *value) {
  FILE *database = fopen(KV_DATABASE_PATH, "a+");
  ErrBox *output = NULL;
  if (database == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "fopen failed\n");
  }

  ErrBox *data = find_key(key);
  if (data->ret_outcome == RETURN_TYPE_SUCCESS) {
    if (data->ret_value != NULL) {
      free(data);
      fclose(database);
      return ErrBox_init(RETURN_TYPE_ERROR, NULL, "They key already exists\n");
    } else {
      // Put into file
      fprintf(database, "%s,%s\n", key, value);
    }
  } else {
    fclose(database);
    return data;
  }

  fclose(database);
  return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
}

/* ErrBox *action_get(const char *key) { */
/*   lol; */
/*   return; */
/* } */
/**/
/* ErrBox *action_delete(const char *key) { */
/*   lol; */
/*   return; */
/* } */
/**/
/* ErrBox *action_clear() { */
/*   lol; */
/*   return; */
/* } */
/**/
/* ErrBox *action_all() { */
/*   lol; */
/*   return; */
/* } */
/**/
int main(int argc, char *argv[]) {
  if (argc == 1) {
    print_usage();
  }

  for (size_t arg_iter = 1; arg_iter < argc; arg_iter++) {
    ErrBox *split_output = tres_commas_split(argv[arg_iter]);
    if (split_output == NULL) {
      fprintf(stdout, "Null ptr received: 1\n");
      return 1;
    }

    Arguments *arguments_struct = NULL;
    if (split_output->ret_outcome == RETURN_TYPE_SUCCESS) {
      arguments_struct = (Arguments *)split_output->ret_value;
      if (arguments_struct == NULL) {
        fprintf(stdout, "Null ptr received: 2\n");
        return 1;
      }

      ErrBox *output = NULL;
      switch (arguments_struct->args[0][0]) {
      case 'p':
        output =
            action_put(arguments_struct->args[1], arguments_struct->args[2]);
        break;
      case 'g':
        /* action_get(arguments_struct->args[1]); */
        break;
      case 'd':
        /* action_delete(arguments_struct->args[1]); */
        break;
      case 'c':
        /* action_clear(); */
        break;
      case 'a':
        /* action_all(); */
        break;
      default:
        fprintf(stdout, "Invalid argument\n");
        return 1;
      }

      if (output->ret_outcome != RETURN_TYPE_SUCCESS) {
        printf("%s\n", output->err_desc);
      }
      free(output);
    } else {
      printf("%s\n", split_output->err_desc);
    }

    for (int args_iter = 0; args_iter < arguments_struct->arg_count;
         args_iter++) {
      free(arguments_struct->args[args_iter]);
    }
    free(arguments_struct->args);
    free(split_output->err_desc);
    free(split_output->ret_value);
    free(split_output);
  }
  return 0;
}