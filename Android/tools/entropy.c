#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>



#define MAX_FILESIZE 10 * 1024 * 1024 // 10MB Max

/* Count how many times each byte value (0-255) appears */
void count_bytes(unsigned char *data, long size, long *freq) {
    for (int i = 0; i < size; i++) {
        freq[data[i]]++;
    }
}


/* Shannon entropy formula:
  H = -sum (p * log2(p)) for all bytes values
  Max entropy = 8.0 bits (perfectly random)
  Lower = more predictable/ structured */

  double calculate_entropy(long *freq, long size) {
    double entropy = 0.0;

    for (int i = 0; i < 256; i++) {
      if (freq[i] == 0) continue;

      double probability = (double)freq[i] / size;
      entropy -= probability * log2(probability);

    }

    return entropy;

  }

  /* Visual bar showing entropy level */
  void print_bar(double entropy) {
    int filled = (int)((entropy / 8.0) * 40);


    printf("  [");
    for (int i = 0; i < 40; i++) {
      if (i < filled) printf("█");
      else printf("░");
    }
    printf("] %.4f / 8.0\n", entropy);

  }

  /* Classify what kind of data this likely is */
  void classify(double entropy) {
    printf("  Classifocation: ");

    if (entropy > 7.9)
      printf("\033[91m[!] ENCRYPTED or COMPRESSED — very high entropy\033[0m\n");
    else if (entropy > 7.0)
        printf("\033[93m[~] Likely encrypted/compressed with some structure\033[0m\n");
    else if (entropy > 5.0)
        printf("\033[96m[*] Mixed data — code, lookup tables, or packed structs\033[0m\n");
    else if (entropy > 3.0)
        printf("\033[92m[+] Structured data — readable format, low randomness\033[0m\n");
    else
        printf("\033[92m[+] Highly structured — text, config, or sparse data\033[0m\n");
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: %s <file>\n", argv[0]);
    return 1;

  }

  /* Opem file in binary mode - 'b' flag matters on some systems */
  FILE *fp = fopen(argv[1], "rb");
  if (!fp) {
    printf("[-] Cannot open file: %s\n", argv[1]);
    return 1;

  }
  /* Get file size by seeking to end */
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET); // seek back to start


  if (size > MAX_FILESIZE) {
    printf("[-] File too large (max 10MB)\n");
    fclose(fp);
    return 1;

  }

  /* Read entire file into memory */
  unsigned char *data = malloc(size);
  if (!data) {
    printf("[-] Memory allocation failed\n");
    fclose(fp);
    return 1;

  }

  fread(data, 1, size, fp);
  fclose(fp);

  /* Count byte frequencies */
  long freq[256];
  memset(freq, 0, sizeof(freq));
  count_bytes(data, size, freq);

  /* Find most and least common bytes */
  int most_common = 0, least_common = 0;
    for (int i = 1; i < 256; i++) {
        if (freq[i] > freq[most_common]) most_common = i;
        if (freq[i] < freq[least_common]) least_common = i;
    }

    /* Calculate and display results */
    double entropy = calculate_entropy(freq, size);

    printf("\n\033[92m[+] File     :\033[0m %s\n", argv[1]);
    printf("\033[92m[+] Size     :\033[0m %ld bytes (%.2f KB)\n", size, size / 1024.0);
    printf("\033[92m[+] Entropy  :\033[0m\n");
    print_bar(entropy);
    classify(entropy);
    printf("\033[92m[+] Most common byte  :\033[0m 0x%02x (%ld times)\n",
           most_common, freq[most_common]);
    printf("\033[92m[+] Least common byte :\033[0m 0x%02x (%ld times)\n\n",
           least_common, freq[least_common]);

    free(data);
    return 0;
}