def frontClockwise(initial):
    clockwise_permutation = (
                0, 1, 27, 3, 4, 28, 6, 7, 29,
                9, 10, 11, 12, 13, 14, 8, 5, 2,
                24, 21, 18, 25, 22, 19, 26, 23, 20,
                42, 39, 36, 30, 31, 32, 33, 34, 35,
                15, 37, 38, 16, 40, 41, 17, 43, 44,
                45, 46, 47, 48, 49, 50, 51, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def frontCounterClockwise(initial):
    clockwise_permutation = (
                0, 1, 17, 3, 4, 16, 6, 7, 15,
                9, 10, 11, 12, 13, 14, 36, 39, 42,
                20, 23, 26, 19, 22, 25, 18, 21, 24,
                2, 5, 8, 30, 31, 32, 33, 34, 35,
                27, 37, 38, 28, 40, 41, 29, 43, 44,
                45, 46, 47, 48, 49, 50, 51, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def backClockwise(initial):
    clockwise_permutation = (
                33, 1, 2, 34, 4, 5, 35, 7, 8,
                6, 3, 0, 12, 13, 14, 15, 16, 17,
                18, 19, 20, 21, 22, 23, 24, 25, 26,
                27, 28, 29, 30, 31, 32, 44, 41, 38, 
                36, 37, 9, 39, 40, 10, 42, 43, 11,
                47, 50, 53, 46, 49, 52, 45, 48, 51
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def backCounterClockwise(initial):
    clockwise_permutation = (
                11, 1, 2, 10, 4, 5, 9, 7, 8,
                38, 41, 44, 12, 13, 14, 15, 16, 17,
                18, 19, 20, 21, 22, 23, 24, 25, 26,
                27, 28, 29, 30, 31, 32, 0, 3, 6,
                36, 37, 35, 39, 40, 34, 42, 43, 33,
                51, 48, 45, 52, 49, 46, 53, 50, 47
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;




def main():
    # 6 Faces 54 cells
    #           Blue - Red - White - Orange - Green - Yellow
    backup = ("B", "B", "B", "B", "B", "B", "B", "B", "B",
                "R", "R", "R", "R", "R", "R", "R", "R", "R",
                "W", "W", "W", "W", "W", "W", "W", "W", "W",
                "O", "O", "O", "O", "O", "O", "O", "O", "O",
                "G", "G", "G", "G", "G", "G", "G", "G", "G",
                "Y", "Y", "Y", "Y", "Y", "Y", "Y", "Y", "Y",
                );
    initial = backup;

    initial = frontClockwise(initial);
    print(initial);
    initial = backup;
    initial = frontCounterClockwise(initial);
    print(initial);
    initial = backup;
    initial = backClockwise(initial);
    print(initial);
    initial = backup;
    initial = backCounterClockwise(initial);
    print(initial);
    initial = backup;



main();

