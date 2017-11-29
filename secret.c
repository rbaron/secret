#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define IRREDUCTIBLE_POLY 0x011b

uint8_t **MULTIPLICATIVE_INVERSE_TABLE = NULL;

uint8_t p_add(uint8_t a, uint8_t b) {
  return a ^ b;
}

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
  // malloc a n rows x (secret_size + 1) cols matrix
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
  uint8_t *res = malloc(strlen(s) * sizeof(uint8_t));
  char buff[3] = {0x00, 0x00, 0x00};
  for (int pos = 0; pos < strlen(s); pos++) {
    //scanf(res[pos], "%02x", s[pos*2]);
    //res[pos] = strtol(
    strncpy(buff, s + pos*2, 2);
    res[pos] = strtoul(buff, NULL, 16);
  }
  return res;
}

int main() {
  srand((unsigned) time(NULL));

  //printf("0x%x\n", time_x(0x57));
  //printf("0x%x\n", time_x(0xae));
  //printf("0x%x\n", time_x(0x47));
  //printf("0x%x\n", time_x(0x8e));


  //uint8_t *poly = make_random_poly(3, 0xff);
  //for (int d = 3; d >= 0; d--) {
  //  printf("0x%x +", poly[d]);
  //}
  //printf("\n");
  //printf("0x%x\n", poly_eval(poly, 3, 0x1));

  uint8_t secret[] = { 0xff, 0x12, 0x3c, 0x00, 0x89 };

  char *s = arr_to_hex_str(secret, 5);
  printf("%s\n", s);

  uint8_t *arr = hex_str_to_arr(s);

  for (int s=0; s < 5; s++) {
    printf("-> 0x%x\n", arr[s]);
  }

  //printf("0x%02x", 0x00);

  //uint8_t **shares = split(secret, 5, 10, 3);

  //uint8_t **shares_subset = (uint8_t **) malloc(3 * sizeof(uint8_t *));
  //shares_subset[0] = shares[0];
  //shares_subset[1] = shares[1];
  //shares_subset[2] = shares[2];

  //uint8_t *reconstructed_secret = join(shares_subset, 5, 3);

  //for (int s=0; s < 5; s++) {
  //  printf("-> 0x%x\n", reconstructed_secret[s]);
  //}

  //for (int p = 0; p <= 255; p++) {
  //  uint8_t inv = p_inv(p);
  //  printf("0x%x * 0x%x = 0x%x\n", p, inv, p_mul(p, inv));
  //}
}
