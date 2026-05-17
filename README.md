# Vacuum Filter
## Project overview
This is a university project for the 2026 course Bioinformatics 1 at FER. In this project we are implementing the Vacuum Filter and using it to search for random subsequences in the E. coli genome and artificially generated data.

## Installation
Clone the repository:
```
git clone https://github.com/yourusername/vacuum-filter.git
cd vacuum-filter
make
```
Download E. coli genome:
```
wget "https://ftp.ncbi.nlm.nih.gov/genomes/all/GCF/000/005/845/GCF_000005845.2_ASM584v2/GCF_000005845.2_ASM584v2_genomic.fna.gz"
gunzip GCF_000005845.2_ASM584v2_genomic.fna.gz 
```

## Usage 
```
./test GCF_000005845.2_ASM584v2_genomic.fna
```

## Input and output formats
Input
- Genome sequence file, FNA/FASTA format
- k values for k-mer extraction

Output
- CSV file with calculated metrics

## Example output
```
Need to add...
```


## Authors
- Matheo Kesar
- Nika Valić

## References 
- Wang et al. Vacuum Filters: More Space-Efficient and Faster Replacement for Bloom and Cuckoo
Filters
(https://github.com/wuwuz/Vacuum-Filter.git)


