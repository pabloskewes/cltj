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
        "output": [4, 5, 1],
        "description": "El caso de prueba principal extraído de la imagen del paper de Barbay.",
    },
    "high_alternation": {
        "input": [
            [1, 4, 7, 10, 13, 16],
            [2, 5, 8, 11, 14, 17],
            [3, 6, 9, 12, 15, 18],
        ],
        "description": "Listas perfectamente intercaladas, diseñadas para maximizar la alternancia.",
    },
    "zero_alternation": {
        "input": [[100, 200, 300], [1, 5, 10, 50], [2, 6, 12, 60]],
        "description": "Una lista siempre está por delante, lo que debería resultar en cero alternancias.",
    },
    "all_identical": {
        "input": [[5, 10, 15], [5, 10, 15], [5, 10, 15]],
        "description": "Todas las listas son idénticas. El líder (con desempate) nunca debería cambiar.",
    },
    "high_density": {
        "input": [
            [1, 2, 3, 4, 8, 9, 10],
            [1, 3, 4, 5, 6, 8, 10],
            [1, 2, 4, 6, 7, 9, 10],
        ],
        "description": "Listas muy similares con alta densidad y múltiples puntos de intersección pequeños (1, 4, 10).",
    },
    "complex_intersection": {
        "input": [
            [3, 5, 10, 12, 18, 25, 30],
            [5, 11, 12, 15, 20, 25, 30],
            [1, 2, 5, 12, 21, 25, 30],
        ],
        "description": "Un caso más realista con múltiples elementos en la intersección (5, 12, 25, 30).",
    },
}
