#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Максимальная длина строки
const int STR_MAX_LEN = 1000;

// Символ переноса строки
const char STR_DELIMITER = '\n';

// Символ конца строки
const char STR_END = '\0';

// Тип для фиктивного аргумента функции. Необходим для перегрузки.
// Сигнатура текстого файла.
struct BIN_FILE_SIGNATURE {};
const BIN_FILE_SIGNATURE BIN_FILE = BIN_FILE_SIGNATURE();

// Тип для фиктивного аргумента функции. Необходим для перегрузки.
// Сигнатура бинарного файла.
struct TXT_FILE_SIGNATURE {};
const TXT_FILE_SIGNATURE TXT_FILE = TXT_FILE_SIGNATURE();

struct Word {
  const char *path;
  size_t offset;
  size_t len;
  size_t weight;
};

// Метаданные файлов
struct Metadata {
  Word *words;
  size_t amount;
};

// Создать экземпляр структуры Word
Word newWord(const char *path = "", size_t offset = 0, size_t len = 0,
             size_t weight = 0) {
  Word word;
  word.path = path;
  word.offset = offset;
  word.len = len;
  word.weight = weight;

  return word;
}

// Создать экземпляр структуры Metadata
Metadata newMetadata(size_t amount = 0) {
  Metadata metadata;
  if (amount != 0)
    metadata.words = new Word[amount];
  else
    metadata.words = nullptr;

  metadata.amount = amount;

  return metadata;
}

// Удалить из кучи данные, хранящиеся в Metadata
void deleteMetadata(Metadata metadata) {
  delete[] metadata.words;
  metadata.words = nullptr;
  metadata.amount = 0;
}

// Добавить в конец 'Metadata' данные строки 'Word'
void pushWord(Metadata &metadata, Word word) {
  Word *newWords = new Word[metadata.amount + 1];

  for (size_t i = 0; i < metadata.amount; i++) {
    newWords[i] = metadata.words[i];
  }

  newWords[metadata.amount] = word;
  delete[] metadata.words;
  metadata.words = newWords;
  metadata.amount++;
}

// Слить две структуры 'Metadata' в одну
void concatMetadata(Metadata &metadata, Metadata additionalMetadata) {
  Word *newWords = new Word[metadata.amount + additionalMetadata.amount];

  for (size_t i = 0; i < metadata.amount; i++) {
    newWords[i] = metadata.words[i];
  }

  for (size_t i = 0; i < additionalMetadata.amount; i++) {
    newWords[metadata.amount + i] = additionalMetadata.words[i];
  }

  deleteMetadata(additionalMetadata);

  delete[] metadata.words;
  metadata.words = newWords;
  metadata.amount += additionalMetadata.amount;
}

// Получить метаданные из бинарного файла
Metadata getMetadataFromFile(BIN_FILE_SIGNATURE _, FILE *file,
                             const char *src) {
  Metadata metadata = newMetadata();

  int offset = 0, len = 0;
  fseek(file, offset, SEEK_SET);

  while (fread(&len, sizeof(int), 1, file)) {
    size_t weight = 0;

    for (size_t i = 0; i < len; i++) {
      char sym;
      fread(&sym, sizeof(char), 1, file);
      weight += sym;
    }

    pushWord(metadata, newWord(src, offset + sizeof(int), len, weight));

    offset += sizeof(int) + len * sizeof(char);
  }

  return metadata;
}

// Получить метаданные из текстового файла
Metadata getMetadataFromFile(TXT_FILE_SIGNATURE _, FILE *file,
                             const char *src) {
  Metadata metadata = newMetadata();

  fseek(file, 0, SEEK_END);
  int fileLen = ftell(file);
  fseek(file, 0, SEEK_SET);

  size_t weight = 0;

  for (int offset = 0, len = 0; offset + len < fileLen;) {
    char symbol = getc(file);

    if (symbol != STR_DELIMITER && !isspace(symbol) &&
        offset + len < fileLen - 1) {
      weight += symbol;
      len++;
      continue;
    };

    if (symbol != STR_DELIMITER && !isspace(symbol)) {
      len++;
    }

    if (len != 0)
      pushWord(metadata, newWord(src, offset, len, weight));

    offset += len + 1;
    len = 0;
    weight = 0;
  }

  return metadata;
}

// Прочитать слово из файла
const char *readWord(FILE *file, Word &word) {
  char *str = new char[word.len + 1];

  fseek(file, word.offset, SEEK_SET);
  fread(str, sizeof(char), word.len, file);

  str[word.len] = '\0';

  return str;
}

// Записать слово в файл
void writeWord(FILE *file, const char *str, bool isLast) {
  fwrite(str, sizeof(char), strlen(str), file);
  if (!isLast)
    fwrite(&STR_DELIMITER, sizeof(char), 1, file);
}

// Открыть текстовый файл
FILE *openFile(TXT_FILE_SIGNATURE _, const char *src) {
  FILE *file;
  if ((file = fopen(src, "r+")) == NULL) {
    printf("Ошибка! Не удалось открыть файл %s", src);
    exit(EXIT_FAILURE);
  }

  return file;
}

// Открыть бинарный файл
FILE *openFile(BIN_FILE_SIGNATURE _, const char *src) {
  FILE *file;
  if ((file = fopen(src, "rb+")) == NULL) {
    printf("Ошибка! Не удалось открыть файл %s", src);
    exit(EXIT_FAILURE);
  }

  return file;
}

// Создать текстовый файл
FILE *createFile(TXT_FILE_SIGNATURE _, const char *src) {
  FILE *file;
  if ((file = fopen(src, "w+")) == NULL) {
    printf("Ошибка! Не удалось создать файл %s", src);
    exit(EXIT_FAILURE);
  }

  return file;
}

// Создать бинарный файл
FILE *createFile(BIN_FILE_SIGNATURE _, const char *src) {
  FILE *file;
  if ((file = fopen(src, "wb+")) == NULL) {
    printf("Ошибка! Не удалось создать файл %s", src);
    exit(EXIT_FAILURE);
  }

  return file;
}

// Отсортировать слова методом слияния
void sortMetadataByMerge(Metadata &metadata, bool isSortByAscending,
                         int leftBound, int rightBound) {
  if (leftBound + 1 >= rightBound)
    return;

  int middleBound = (leftBound + rightBound) / 2;

  // Тривиальный случай

  sortMetadataByMerge(metadata, isSortByAscending, leftBound, middleBound);
  sortMetadataByMerge(metadata, isSortByAscending, middleBound, rightBound);

  // Декомпозиция общего случая

  Metadata metadataBlock = newMetadata(rightBound - leftBound);

  int leftI = 0;
  int rightI = 0;

  while (leftBound + leftI < middleBound && middleBound + rightI < rightBound) {
    if (!isSortByAscending && metadata.words[leftBound + leftI].weight >
                                  metadata.words[middleBound + rightI].weight) {
      metadataBlock.words[leftI + rightI] = metadata.words[leftBound + leftI];
      leftI++;
    } else if (isSortByAscending &&
               metadata.words[leftBound + leftI].weight <=
                   metadata.words[middleBound + rightI].weight) {
      metadataBlock.words[leftI + rightI] = metadata.words[leftBound + leftI];
      leftI++;
    } else {
      metadataBlock.words[leftI + rightI] =
          metadata.words[middleBound + rightI];
      rightI++;
    }
  }

  while (leftBound + leftI < middleBound) {
    metadataBlock.words[leftI + rightI] = metadata.words[leftBound + leftI];
    leftI++;
  }

  while (middleBound + rightI < rightBound) {
    metadataBlock.words[leftI + rightI] = metadata.words[middleBound + rightI];
    rightI++;
  }

  for (int i = 0; i < leftI + rightI; i++) {
    metadata.words[leftBound + i] = metadataBlock.words[i];
  }

  deleteMetadata(metadataBlock);
}

// Отсортировать строки по убыванию
void sortByDescending(Metadata &metadata) {
  sortMetadataByMerge(metadata, false, 0, metadata.amount);
}

// Отсортировать сроки по возрастанию
void sortByAscending(Metadata &metadata) {
  sortMetadataByMerge(metadata, true, 0, metadata.amount);
}

// Отсортировать строки в файлах и записать в другой файл
template <typename Signature>
void sortFilesWords(Signature signature, const char *dest,
                    void (*sortMetadata)(Metadata &), ...) {
  va_list args;
  va_start(args, sortMetadata);

  Metadata metadata = newMetadata();

  const char *src;
  while ((src = va_arg(args, const char *)) != NULL) {
    FILE *file = openFile(signature, src);
    concatMetadata(metadata, getMetadataFromFile(signature, file, src));
    fclose(file);
  }

  sortMetadata(metadata);

  FILE *destFile = createFile(signature, dest);

  for (int i = 0; i < metadata.amount; i++) {
    Word word = metadata.words[i];

    FILE *file = openFile(signature, word.path);
    const char *str = readWord(file, word);
    writeWord(destFile, str, i >= metadata.amount - 1);

    delete[] str;
    fclose(file);
  }

  va_end(args);
  fclose(destFile);
  deleteMetadata(metadata);
}

// Функции для теста

// Вывести содержимое бинарного файла
void printBinFile(const char *path) {
  FILE *file = openFile(BIN_FILE, path);

  printf("Содержимое бинарного файла %s:\n", path);

  int len = 0;
  while (fread(&len, sizeof(int), 1, file)) {
    for (int i = 0; i < len; i++)
      printf("%c", fgetc(file));
    printf("\n");
  }

  fclose(file);
}

// Создать бинарный файл из текстового
void createBinFileFromTxt(const char *destPath, const char *srcPath) {
  FILE *destFile = createFile(BIN_FILE, destPath);
  FILE *srcFile = openFile(TXT_FILE, srcPath);

  Metadata metadata = getMetadataFromFile(TXT_FILE, srcFile, srcPath);

  for (int i = 0; i < metadata.amount; i++) {
    Word word = metadata.words[i];
    const char *str = readWord(srcFile, word);

    int len = (int)word.len;

    fwrite(&len, sizeof(int), 1, destFile);
    fwrite(str, sizeof(char), strlen(str), destFile);

    delete[] str;
  }

  fclose(srcFile);
  fclose(destFile);
}

int main() {
  /*
  createBinFileFromTxt("test_1_src_1.bin", "test_1_src_1.txt");
  createBinFileFromTxt("test_1_src_2.bin", "test_1_src_2.txt");
  createBinFileFromTxt("test_1_src_3.bin", "test_1_src_3.txt");

  createBinFileFromTxt("test_2_src_1.bin", "test_2_src_1.txt");
  createBinFileFromTxt("test_2_src_2.bin", "test_2_src_2.txt");
  createBinFileFromTxt("test_2_src_3.bin", "test_2_src_3.txt");

  createBinFileFromTxt("test_3_src_1.bin", "test_3_src_1.txt");
  createBinFileFromTxt("test_3_src_2.bin", "test_3_src_2.txt");
  */

  sortFilesWords(TXT_FILE, "test_1_dest.txt", sortByAscending,
                 "test_1_src_1.txt", "test_1_src_2.txt", "test_1_src_3.txt",
                 NULL);
  sortFilesWords(BIN_FILE, "test_1_dest.bin", sortByAscending,
                 "test_1_src_1.bin", "test_1_src_2.bin", "test_1_src_3.bin",
                 NULL);

  sortFilesWords(TXT_FILE, "test_2_dest.txt", sortByDescending,
                 "test_2_src_1.txt", "test_2_src_2.txt", "test_2_src_3.txt",
                 NULL);
  sortFilesWords(BIN_FILE, "test_2_dest.bin", sortByDescending,
                 "test_2_src_1.bin", "test_2_src_2.bin", "test_2_src_3.bin",
                 NULL);

  sortFilesWords(TXT_FILE, "test_3_dest.txt", sortByDescending,
                 "test_3_src_1.txt", "test_3_src_2.txt", NULL);
  sortFilesWords(BIN_FILE, "test_3_dest.bin", sortByDescending,
                 "test_3_src_1.bin", "test_3_src_2.bin", NULL);

  return EXIT_SUCCESS;
}
