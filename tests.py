import random

def generate_file(filename, count, overlap_ratio=0.5):
    with open(filename, 'w') as f:
        for i in range(count):
            # 50% chance to be a common IP (192.168.0.x), 50% random
            if random.random() < overlap_ratio:
                f.write(f"192.168.0.{i % 255}\n")
            else:
                f.write(f"10.0.{random.randint(0,255)}.{random.randint(0,255)}\n")

generate_file("2_1k.txt", 1000)
generate_file("2_10k.txt", 10000)
generate_file("2_50k.txt", 50000)
generate_file("2_100k.txt", 100000)
