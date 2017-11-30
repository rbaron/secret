#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define IRREDUCTIBLE_POLY 0x011b

uint8_t **MULTIPLICATIVE_INVERSE_TABLE = NULL;

// Add two polynomials in GF(2^8)
uint8_t p_add(uint8_t a, uint8_t b) {
  return a ^ b;
}

// Multiply a polynomial by x in GF(2^8)
uint8_t time_x(uint8_t a) {
  if ((a >> 7) & 0x1) {
    return (a << 1) ^ IRREDUCTIBLE_POLY;
  } else {
    return (a << 1);
  }
}

uint8_t time_x_power(uint8_t a, uint8_t x_power) {
  uint8_t res = a;
  for (; x_power > 0; x_power--) {
    res = time_x(res);
  }
  return res;
}

// Multiply two polynomials in GF(2^8)
uint8_t p_mul(uint8_t a, uint8_t b) {
  uint8_t res = 0;
  for (int degree = 7; degree >= 0; degree--) {
    if ((b >> degree) & 0x1) {
      res = p_add(res, time_x_power(a, degree));
    }
  }
  return res;
}

uint8_t p_inv(uint8_t a) {

  // Build the table so that table[a][1] = inv(a)
  if (MULTIPLICATIVE_INVERSE_TABLE == NULL) {
    MULTIPLICATIVE_INVERSE_TABLE = (uint8_t **) malloc(256 * sizeof(uint8_t *));
    for (int row = 0; row < 256; row++) {
      MULTIPLICATIVE_INVERSE_TABLE[row] = (uint8_t *) malloc(256 * sizeof(uint8_t));

      for (int col = 0; col < 256; col++) {
        MULTIPLICATIVE_INVERSE_TABLE[row][p_mul(row, col)] = col;
      }
    }
  }

  return MULTIPLICATIVE_INVERSE_TABLE[a][1];
}

// Divide two polynomials in GF(2^8)
uint8_t p_div(uint8_t a, uint8_t b) {
  return p_mul(a, p_inv(b));
}

uint8_t rand_byte() {
  return rand() % 0xff;
}

uint8_t * make_random_poly(int degree, uint8_t secret) {
  uint8_t *poly = malloc((degree + 1) * sizeof(uint8_t));
  for (; degree > 0; degree--) {
    poly[degree] = rand_byte();
  }
  poly[0] = secret;
  return poly;
}

uint8_t poly_eval(uint8_t *poly, int degree , uint8_t x) {
  uint8_t res = 0;
  for (; degree >= 0; degree--) {
    uint8_t coeff = poly[degree];
    uint8_t term = 0x01;
    for (int times = degree; times > 0; times--) {
      term = p_mul(term, x);
    }
    res = p_add(res, p_mul(coeff, term));
  }
  return res;
}

// Interpolate a (k-1) degree polynomial and evaluate it at x = 0
uint8_t poly_interpolate(uint8_t *xs, uint8_t *ys, int k) {
  uint8_t res = 0;

  for (int j = 0; j < k; j++) {
    uint8_t prod = 0x01;
    for (int m = 0; m < k; m++) {
      if (m != j) {
        prod = p_mul(prod, p_div(xs[m], p_add(xs[m], xs[j])));
      }
    }
    res = p_add(res, p_mul(ys[j], prod));
  }
  return res;
}

uint8_t ** split(uint8_t *secret, int secret_size, int n, int k) {
  // n rows x (secret_size + 1) cols matrix
  uint8_t **shares = malloc(n * sizeof(uint8_t *));
  for (int i = 0; i < n; i++) {
    shares[i] = malloc((secret_size + 1) * sizeof(uint8_t));

    // x
    shares[i][0] = rand_byte();
  }

  for (int secret_idx = 0; secret_idx < secret_size; secret_idx++) {
    uint8_t *poly = make_random_poly(k-1, secret[secret_idx]);

    // Evaluate poly on every one of the n x points
    for (int i = 0; i < n; i++) {
      shares[i][secret_idx + 1] = poly_eval(poly, k-1, shares[i][0]);
    }
  }

  return shares;
}

uint8_t * join(uint8_t **shares, int secret_size, int k) {
  uint8_t *secret = malloc(secret_size * sizeof(uint8_t));

  for (int secret_idx = 1; secret_idx <= secret_size; secret_idx++) {
    uint8_t *xs = (uint8_t *) malloc(k * sizeof(uint8_t));
    uint8_t *ys = (uint8_t *) malloc(k * sizeof(uint8_t));
    for (int i = 0; i < k; i++) {
      xs[i] = shares[i][0];
      ys[i] = shares[i][secret_idx];

      secret[secret_idx-1] = poly_interpolate(xs, ys, k);
    }
  }

  return secret;
}

char * arr_to_hex_str(uint8_t *arr, int arr_size) {
  char *out = malloc(2 * arr_size + 1);
  for (int pos = 0; pos < arr_size; pos++) {
    sprintf(out + 2*pos, "%02x", arr[pos]);
  }
  out[2 * arr_size + 1] = 0x00;
  return out;
}

uint8_t * hex_str_to_arr(const char *s) {
  // / 2 ?
  uint8_t *res = malloc(strlen(s) * sizeof(uint8_t));
  char buff[3] = {0x00, 0x00, 0x00};
  for (int pos = 0; pos < strlen(s); pos++) {
    strncpy(buff, s + pos*2, 2);
    res[pos] = strtoul(buff, NULL, 16);
  }
  return res;
}

int main(int argc, char *argv[]) {
  srand((unsigned) time(NULL));

  if (strcmp(argv[1], "split") == 0) {
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    char buff[1024];
    printf("Enter your secret: ");

    if(fgets(buff, 1024, stdin) != NULL) {
      int secret_size = strlen(buff);
      uint8_t **shares = split((uint8_t *) buff, secret_size, n, k);

      printf("Shares:\n");
      for (int row = 0; row < n; row++) {
        printf("%s\n", arr_to_hex_str(shares[row], secret_size + 1));
      }
    }
  } else if (strcmp(argv[1], "join") == 0) {
    int k = argc - 2;
    int secret_size = strlen(argv[2]) - 1;
    uint8_t **shares = (uint8_t **) malloc(k * sizeof(uint8_t *));

    for (int i=0; i<k; i++) {
      shares[i] = hex_str_to_arr(argv[i + 2]);
    }

    uint8_t *reconstructed_secret = join(shares, secret_size, k);

    printf("\nReconstructed secret: %s\n", (char *) reconstructed_secret);
  } else {
    printf(
      "usage: secret\n\n"
      "1. Split a secret in n shares, so k are needed to reconstruct it:\n"
      "$ secret split 10 3\n\n"
      "2. Join k shares to reconstruct the original secret::\n"
      "$ secret join SHARE_1 SHARE_2 ... SHARE_K\n\n"
    );
  }

  return 0;
}
