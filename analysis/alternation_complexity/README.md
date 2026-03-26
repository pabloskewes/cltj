# Alternation Complexity

Prototyping and visualization of the alternation complexity ($\delta$) partition certificate algorithm for sorted list intersection.

## Context

The alternation complexity $\delta$ of an instance $(A_1, \dots, A_k)$ is the minimum number of intervals that form a partition certificate. It provides a theoretical lower bound for intersection algorithms.

This module implements the algorithm in Python to validate the logic before integrating it into the C++ `IntersectionStats` instrumentation (`alternation_complexity.hpp`).

## Usage

The primary entry point for interactive exploration and visualization is the notebook:
- [`algorithm_design.ipynb`](algorithm_design.ipynb)