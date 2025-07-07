test_cases = {
    "paper_example": {
        "input": [
            [9],
            [1, 2, 9, 11],
            [3, 9, 12, 13],
            [9, 14, 15, 16],
            [4, 10, 17, 18],
            [5, 6, 7, 10],
            [8, 10, 19, 20],
        ],
        "output": [4] * 8 + [5] + [1] * 11,
        "description": "Ejemplo del paper de Barbay. Intersección vacía, δ=3.",
    },
    "perfectly_interleaved": {
        "input": [
            [1, 4, 7, 10, 13, 16],
            [2, 5, 8, 11, 14, 17],
            [3, 6, 9, 12, 15, 18],
        ],
        "output": [3, 3, 2, 2, 1, 1] * 3,
        "description": "Listas perfectamente intercaladas. Intersección vacía, máxima alternancia posible.",
    },
    "single_responsible": {
        "input": [[1, 2, 3], [5, 6, 7], [9, 10, 11]],
        "output": [3] * 8 + [2] * 3,
        "description": "Listas completamente separadas. Intersección vacía, δ=2.",
    },
    "all_identical": {
        "input": [[3, 5, 7], [3, 5, 7], [3, 5, 7]],
        "output": [0, 1, 0, 1, 0],
        "description": "Todas las listas idénticas. Intersección completa: {3, 5, 7}, δ=5.",
    },
    "simple_intersection": {
        "input": [
            [1, 3, 5, 7],
            [2, 3, 6, 7],
            [3, 4, 7, 8],
        ],
        "output": [3, 3, 0, 2, 2, 1, 0, 1],
        "description": "Intersección pequeña: {3, 7}. Mezcla de singletons e intervalos. δ=6.",
    },
    "two_lists_only": {
        "input": [
            [1, 3, 5, 7, 9],
            [2, 4, 6, 8, 10],
        ],
        "output": [2, 1] * 5,
        "description": "Solo dos listas intercaladas. Intersección vacía, δ=10.",
    },
    "dense_overlap": {
        "input": [
            [1, 2, 4, 5, 6],
            [1, 3, 4, 6, 7],
            [2, 4, 5, 6, 8],
        ],
        "output": [3, 2, 1, 0, 2, 0, 1, 1],
        "description": "Listas con mucho solapamiento. Intersección: {4, 6}. δ=7.",
    },
}
