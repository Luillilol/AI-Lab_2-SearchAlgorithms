from collections import deque

"""
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
                29, 37, 38, 28, 40, 41, 27, 43, 44,
                45, 46, 47, 48, 49, 50, 51, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def backClockwise(initial):
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

def backCounterClockwise(initial):
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

def leftClockwise(initial):
    clockwise_permutation = (
                6, 3, 0, 7, 4, 1, 8, 5, 2,
                53, 10 ,11, 50, 13, 14, 47, 16, 17,
                9, 19, 20, 12, 22, 23, 15, 25, 26,
                18, 28, 29, 21, 31, 32, 24, 34, 35,
                36, 37, 38, 39, 40, 41, 42, 43, 44,
                45, 46, 33, 48, 49, 30, 51, 52, 27
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def leftCounterClockwise(initial):
    clockwise_permutation = (
                2, 5, 8, 1, 4, 7, 0, 3, 6,
                18, 10, 11, 21, 13, 14, 24, 16, 17,
                27, 19, 20, 30, 22, 23, 33, 25, 26,
                53, 28, 29, 50, 31, 32, 47, 34, 35,
                36, 37, 38, 39, 40, 41, 42, 43, 44,
                45, 46, 15, 48, 49, 12, 51, 52, 9
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;


def rightClockwise(initial):
    clockwise_permutation = (
                0, 1, 2, 3, 4, 5, 6, 7, 8,
                9, 10, 20, 12, 13, 23, 15, 16, 26,
                18, 19, 29, 21, 22, 32, 24, 25, 35,
                27, 28, 51, 30, 31, 48, 33, 34, 45,
                42, 39, 36, 43, 40, 37, 44, 41, 38,
                17, 46, 47, 14, 49, 50, 11, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def rightCounterClockwise(initial):
    clockwise_permutation = (
                0, 1, 2, 3, 4, 5, 6, 7, 8,
                9, 10, 51, 12, 13, 48, 15, 16, 45,
                18, 19, 11, 21, 22, 14, 24, 25, 17,
                27, 28, 20, 30, 31, 23, 33, 34, 26,
                38, 41, 44, 37, 40, 43, 36, 39, 42,
                35, 46, 47, 32, 49, 50, 29, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;


def upClockwise(initial):
    clockwise_permutation = (
                18, 19, 20, 3, 4, 5, 6, 7, 8,
                15, 12, 9, 16, 13, 10, 17, 14, 11,
                36, 37, 38, 21, 22, 23, 24, 25, 26,
                27, 28, 29, 30, 31, 32, 33, 34, 35,
                45, 46, 47, 39, 40, 41, 42, 43, 44,
                0, 1, 2, 48, 49, 50, 51, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def upCounterClockwise(initial):
    clockwise_permutation = (
                45, 46, 47, 3, 4, 5, 6, 7, 8,
                11, 14, 17, 10, 13, 16, 9, 12, 15,
                0, 1, 2, 21, 22, 23, 24, 25, 26,
                27, 28, 29, 30, 31, 32, 33, 34, 35,
                18, 19, 20, 39, 40, 41, 42, 43, 44,
                36, 37, 38, 48, 49, 50, 51, 52, 53
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;


def downClockwise(initial):
    clockwise_permutation = (
                0, 1, 2, 3, 4, 5, 51, 52, 53,
                9, 10, 11, 12, 13, 14, 15, 16, 17,
                18, 19, 20, 21, 22, 23, 6, 7, 8,
                33, 30, 27, 34, 31, 28, 35, 32, 29,
                36, 37, 38, 39, 40, 41, 24, 25, 26,
                45, 46, 47, 48, 49, 50, 42, 43, 44
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;

def downCounterClockwise(initial):
    clockwise_permutation = (
                0, 1, 2, 3, 4, 5, 24, 25, 26,
                9, 10, 11, 12, 13, 14, 15, 16, 17, 
                18, 19, 20, 21, 22, 23, 42, 43, 44,
                29, 32, 35, 28, 31, 34, 27, 30, 33,
                36, 37, 38, 39, 40, 41, 51, 52, 53,
                45, 46, 47, 48, 49, 50, 6, 7, 8
            );
    new_Initial = tuple(initial[i] for i in clockwise_permutation);
    return new_Initial;
"""
PERMUTATIONS = {
    "F":(0, 1, 27, 3, 4, 28, 6, 7, 29, 9, 10, 11, 12, 13, 14, 8, 5, 2, 24, 21, 18, 25, 22, 19, 26, 23, 20, 42, 39, 36, 30, 31, 32, 33, 34, 35, 15, 37, 38, 16, 40, 41, 17, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53),
    "F'":(0, 1, 17, 3, 4, 16, 6, 7, 15, 9, 10, 11, 12, 13, 14, 36, 39, 42, 20, 23, 26, 19, 22, 25, 18, 21, 24, 2, 5, 8, 30, 31, 32, 33, 34, 35, 29, 37, 38, 28, 40, 41, 27, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53),
    "B":(11, 1, 2, 10, 4, 5, 9, 7, 8, 38, 41, 44, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 0, 3, 6, 36, 37, 35, 39, 40, 34, 42, 43, 33, 51, 48, 45, 52, 49, 46, 53, 50, 47),
    "B'":(33, 1, 2, 34, 4, 5, 35, 7, 8, 6, 3, 0, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 44, 41, 38, 36, 37, 9, 39, 40, 10, 42, 43, 11, 47, 50, 53, 46, 49, 52, 45, 48, 51),
    "L":(6, 3, 0, 7, 4, 1, 8, 5, 2, 53, 10 ,11, 50, 13, 14, 47, 16, 17, 9, 19, 20, 12, 22, 23, 15, 25, 26, 18, 28, 29, 21, 31, 32, 24, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 33, 48, 49, 30, 51, 52, 27),
    "L'":(2, 5, 8, 1, 4, 7, 0, 3, 6, 18, 10, 11, 21, 13, 14, 24, 16, 17, 27, 19, 20, 30, 22, 23, 33, 25, 26, 53, 28, 29, 50, 31, 32, 47, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 15, 48, 49, 12, 51, 52, 9,),
    "R":(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 12, 13, 23, 15, 16, 26, 18, 19, 29, 21, 22, 32, 24, 25, 35, 27, 28, 51, 30, 31, 48, 33, 34, 45, 42, 39, 36, 43, 40, 37, 44, 41, 38, 17, 46, 47, 14, 49, 50, 11, 52, 53),
    "R'":(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 51, 12, 13, 48, 15, 16, 45, 18, 19, 11, 21, 22, 14, 24, 25, 17, 27, 28, 20, 30, 31, 23, 33, 34, 26, 38, 41, 44, 37, 40, 43, 36, 39, 42, 35, 46, 47, 32, 49, 50, 29, 52, 53),
    "U":(18, 19, 20, 3, 4, 5, 6, 7, 8, 15, 12, 9, 16, 13, 10, 17, 14, 11, 36, 37, 38, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 45, 46, 47, 39, 40, 41, 42, 43, 44, 0, 1, 2, 48, 49, 50, 51, 52, 53),
    "U'":(45, 46, 47, 3, 4, 5, 6, 7, 8, 11, 14, 17, 10, 13, 16, 9, 12, 15, 0, 1, 2, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 18, 19, 20, 39, 40, 41, 42, 43, 44, 36, 37, 38, 48, 49, 50, 51, 52, 53),
    "D":(0, 1, 2, 3, 4, 5, 51, 52, 53, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 6, 7, 8, 33, 30, 27, 34, 31, 28, 35, 32, 29, 36, 37, 38, 39, 40, 41, 24, 25, 26, 45, 46, 47, 48, 49, 50, 42, 43, 44),
    "D'":(0, 1, 2, 3, 4, 5, 24, 25, 26, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 42, 43, 44, 29, 32, 35, 28, 31, 34, 27, 30, 33, 36, 37, 38, 39, 40, 41, 51, 52, 53, 45, 46, 47, 48, 49, 50, 6, 7, 8)

}

INVERSE = {
    "F":"F'",
    "F'":"F",
    "B":"B'",
    "B'":"B",
    "L":"L'",
    "L'":"L",
    "R":"R'",
    "R'":"R",
    "U":"U'",
    "U'":"U",
    "D":"D'",
    "D'":"D"
}

def get_neighbors(state):
    neighbors = []
    #for in to iterate in PERMUTATION and get all neighbors
    for move_name, permutation in PERMUTATIONS.items():
        #new_Initial = tuple(initial[i] for i in clockwise_permutation);
        new_state = tuple(state[i] for i in permutation)
        neighbors.append((move_name, new_state))
    return neighbors


def bidirectionalBFS(initial_state, final_state, PERMUTATIONS):
    #Initialize deque for forward and backward
    forward_queue = deque([(initial_state, [])])
    backward_queue = deque([(final_state, [])])

    #Initialize visited dictionares {state_tuple : [moves_taken]}
    forward_visited = {initial_state:[]}
    backward_visited = {final_state:[]}
    #main loop to iterate 
    while(forward_queue and backward_queue):
        # pop element from forward_queue
        
        # FOR FORWWARD
        current_forward_state, forward_path = forward_queue.popleft();
        #visit all neighbors from current_forward_state
        for move_name, next_forward_state in get_neighbors(current_forward_state):             
            #If neighbor hasn't been visited yet
            if next_forward_state not in forward_visited:
                #Build te new path
                new_forward_path = forward_path + [move_name]
                #Add the state to visited and
                forward_visited[next_forward_state] = new_forward_path
                forward_queue.append((next_forward_state, new_forward_path))
                if next_forward_state in backward_visited:
                    # Intersected path
                    backpath = backward_visited[next_forward_state]
                    inverted_backpath = [INVERSE[m] for m in reversed(backpath)]
                    return new_forward_path + inverted_backpath
        
        # FOR BACKWARD
        current_backward_state, backward_path = backward_queue.popleft();
        #visit all neighbors from current_backward_state
        for move_name, next_backward_state in get_neighbors(current_backward_state):             
            #If neighbor hasn't been visited yet
            if next_backward_state not in backward_visited:
                #Build te new path
                new_backward_path = backward_path + [move_name]
                #Add the state to visited and
                backward_visited[next_backward_state] = new_backward_path
                backward_queue.append((next_backward_state, new_backward_path))
                if next_backward_state in forward_visited:
                    #exit de todo y ya ha encontrado camino
                    forward_path = forward_visited[next_backward_state]
                    inverted_backpath = [INVERSE[m] for m in reversed(new_backward_path)]
                    return forward_path + inverted_backpath
    


        




def main():
    # 6 Faces 54 cells
    #           Blue - Red - White - Orange - Green - Yellow
    goal_state = ("B", "B", "B", "B", "B", "B", "B", "B", "B",
                "R", "R", "R", "R", "R", "R", "R", "R", "R",
                "W", "W", "W", "W", "W", "W", "W", "W", "W",
                "O", "O", "O", "O", "O", "O", "O", "O", "O",
                "G", "G", "G", "G", "G", "G", "G", "G", "G",
                "Y", "Y", "Y", "Y", "Y", "Y", "Y", "Y", "Y",
                );
    """
    initial_state = ("R", "Y", "O", "B", "B", "O", "B", "B", "B",
                    "W", "W", "B", "R", "R", "B", "B", "B", "W",
                    "W", "W", "O", "W", "W", "O", "R", "R", "O",
                    "W", "W", "Y", "O", "O", "Y", "O", "O", "Y",
                    "G", "R", "R", "G", "G", "G", "G", "G", "G",
                    "Y", "G", "G", "R", "Y", "Y", "R", "Y", "Y",
                );
    """
    # F, U, R', D, B', L, U, D', F', R, U, L', U, B, F 15
    initial_state = ("G", "G", "W", "G", "B", "W", "Y", "Y", "Y",
                    "W", "Y", "R", "W", "R", "R", "B", "B", "G",
                    "R", "O", "O", "B", "W", "W", "B", "G", "W",
                    "O", "O", "G", "O", "O", "B", "R", "R", "B",
                    "Y", "G", "G", "R", "G", "W", "R", "R", "W",
                    "Y", "B", "O", "O", "Y", "Y", "O", "Y", "B",
                );
    
    """
    # U, R, F, U, F, R     
    initial_state = ("W", "W", "G", "B", "B", "G", "B", "B", "G",
                    "B", "R", "R", "B", "R", "G", "Y", "O", "Y",
                    "O", "W", "R", "O", "W", "Y", "O", "G", "B",
                    "W", "R", "R", "O", "O", "R", "O", "O", "W",
                    "G", "W", "W", "G", "G", "B", "Y", "Y", "B",
                    "G", "W", "O", "R", "Y", "Y", "R", "Y", "Y",
                );
    """
    
    
    #initial = goal_state;

    #initial = get_neighbors(initial);
    print(bidirectionalBFS(initial_state, goal_state, PERMUTATIONS))



main();

