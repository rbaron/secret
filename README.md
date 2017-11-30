# secret

A minimalistic C implementation of [Shamir's secret sharing](https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing) scheme.

[![asciicast](https://asciinema.org/a/q8jt1lugfYmuTg36oa2Nb7Kej.png)](https://asciinema.org/a/q8jt1lugfYmuTg36oa2Nb7Kej)

## Building

`$ gcc -o secret secret.c`

## Usage

- Splitting a secret in `n = 5` shares, so any `k = 2` shares are needed to reconstruct it:

```
% ./secret split 5 2
Enter your secret: this is a secret!
Shares:
3cb1c0dcc4b246f103c7060e6de4c9ee2c3357
15d09d8380eb2c7033db80ea01d3d783d5f018
8c6d3e21a24a19bd2bd5c3983745d838241cb2
7f660453f1e04504b10886bab3234d98ff2eb6
b5fe793fc5bf0dad32e699a9c816b266878052
```

- Joining `k = 2` shares to reconstruct the original secret:

```
./secret join 15d09d8380eb2c7033db80ea01d3d783d5f018 b5fe793fc5bf0dad32e699a9c816b266878052

Reconstructed secret: this is a secret!
```

## License

MIT.
