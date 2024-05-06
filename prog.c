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
      output->key = malloc((data_iter + 1) * sizeof(char));
      output->value = malloc((strlen(data) - data_iter) * sizeof(char));

      memset(output->key, '\0', data_iter + 1);
      memset(output->value, '\0', (strlen(data) - data_iter));

      strncpy(output->key, data, data_iter);
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
  while (getline(&line, &line_size, database) != -1) {
    if (line[0] == '\0') {
      break;
    }

    ErrBox *data = KVBox_init_from_data(line);
    if (data->ret_outcome == RETURN_TYPE_SUCCESS) {
      KVBox *kv_data = data->ret_value;
      if (!strcmp(kv_data->key, key)) {
        fclose(database);
        free(line);
        line = NULL;
        return data;
      } else {
        free(kv_data->value);
        free(kv_data->key);
        ErrBox_free(data);
      }
    } else {
      fclose(database);
      free(line);
      line = NULL;
      return data;
    }
  }

  fclose(database);
  free(line);
  // if the file is empty or function did not find the key
  // it returns SUCCESS with no value
  return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
}

ErrBox *action_put(const char *key, const char *value) {
  ErrBox *data = find_key(key);

  FILE *database = fopen(KV_DATABASE_PATH, "a+");
  ErrBox *output = NULL;
  if (database == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "fopen failed\n");
  }

  if (data->ret_outcome == RETURN_TYPE_SUCCESS) {
    if (data->ret_value != NULL) {
      free(data);
      fclose(database);
      return ErrBox_init(RETURN_TYPE_ERROR, NULL, "The key already exists\n");
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

ErrBox *action_delete(const char *key) {
  ErrBox *found_key = find_key(key);

  if (found_key != NULL && found_key->ret_outcome != RETURN_TYPE_ERROR &&
      found_key->ret_value != NULL) {
    free(((KVBox *)found_key->ret_value)->key);
    free(((KVBox *)found_key->ret_value)->value);
    ErrBox_free(found_key);

    // delete key, this is the fastest method to do I can think of without
    // having to rewrite the whole program
    const char *temp_kv_database_name = "temp_kv_database.txt";
    FILE *new_kv_database = fopen(temp_kv_database_name, "w");
    FILE *old_kv_database = fopen(KV_DATABASE_PATH, "r");
    if (new_kv_database == NULL || old_kv_database == NULL) {
      return ErrBox_init(RETURN_TYPE_ERROR, NULL, "fopen failed\n");
    }

    size_t line_size = 0;
    char *line = NULL;
    while (getline(&line, &line_size, old_kv_database) != -1) {
      char *line_copy = malloc((strlen(line) + 1) * sizeof(char));
      strcpy(line_copy, line);
      // Could've used it earlier, but oh well, happens
      char *token = strsep(&line_copy, ",");
      if (strcmp(token, key) == 0) {
        free(token);
        continue;
      }
      fprintf(new_kv_database, "%s", line);
      free(token);
    }
    free(line);
    fclose(old_kv_database);
    fclose(new_kv_database);

    if (remove(KV_DATABASE_PATH) == -1) {
      return ErrBox_init(RETURN_TYPE_ERROR, NULL, "remove failed\n");
    }
    if (rename(temp_kv_database_name, KV_DATABASE_PATH) != 0) {
      return ErrBox_init(RETURN_TYPE_ERROR, NULL, "rename failed\n");
    }
  } else if (found_key->ret_outcome == RETURN_TYPE_ERROR) {
    return found_key;
  } else {
    ErrBox_free(found_key);
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "Key was not found\n");
  }

  return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
}

ErrBox *action_clear() {
  FILE *file = fopen(KV_DATABASE_PATH, "w");
  if (file == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "Fopen failed\n");
  }

  return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
}

ErrBox *action_all() {
  FILE *database = fopen(KV_DATABASE_PATH, "r");
  if (database == NULL) {
    return ErrBox_init(RETURN_TYPE_ERROR, NULL, "fopen failed\n");
  }

  size_t line_size = 0;
  char *line = NULL;
  while (getline(&line, &line_size, database) != -1) {
    ErrBox *data = KVBox_init_from_data(line);
    if (data->ret_outcome == RETURN_TYPE_SUCCESS) {
      KVBox *kv_data = data->ret_value;
      fprintf(stdout, "Key: %s, Value: %s", kv_data->key, kv_data->value);
      free(kv_data->value);
      free(kv_data->key);
    } else {
      fclose(database);
      free(line);
      return data;
    }

    ErrBox_free(data);
  }

  fclose(database);
  free(line);
  // if the file is empty or function did not find the key
  // it returns SUCCESS with no value
  return ErrBox_init(RETURN_TYPE_SUCCESS, NULL, NULL);
}

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

    // check if file exists, create if not
    FILE *file = fopen(KV_DATABASE_PATH, "r");
    if (file == NULL) {
      fclose(fopen(KV_DATABASE_PATH, "a"));
    } else {
      fclose(file);
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
        output = action_get(arguments_struct->args[1]);
        break;
      case 'd':
        output = action_delete(arguments_struct->args[1]);
        break;
      case 'c':
        output = action_clear();
        break;
      case 'a':
        output = action_all();
        break;
      default:
        fprintf(stdout, "Invalid argument\n");
        return 1;
      }
      if (output != NULL) {
        if (output->ret_outcome != RETURN_TYPE_SUCCESS) {
          printf("%s\n", output->err_desc);
        }
        /* ErrBox_free(output); */
      }
    } else {
      printf("%s\n", split_output->err_desc);
    }

    for (int args_iter = 0; args_iter < arguments_struct->arg_count;
         args_iter++) {
      free(arguments_struct->args[args_iter]);
    }
    free(arguments_struct->args);
    ErrBox_free(split_output);
  }
  return 0;
}
