import random

if __name__ == "__main__":
    num_words = 10000
    seq = list([x for x in range(1, num_words+1)])
    seq = seq + seq

    seq.append(0)

    random.shuffle(seq)

    # This will generate a file with approximatly 100KB+
    with open('test.txt', 'w') as f:
        for x in seq:
            f.write(f'data-{x}\n')

