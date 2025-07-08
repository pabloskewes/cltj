from dataclasses import dataclass


@dataclass
class Interval:
    left: int | float
    right: int | float

    def __post_init__(self):
        if self.left > self.right:
            raise ValueError("Left bound must be less than right bound")

    def __repr__(self):
        if self.left == self.right:
            return f"{{{self.left}}}"

        inf_symbol = "∞"
        left_bracket = "(" if self.left == NEG_INF else "["
        right_bracket = ")"
        left_value = self.left if self.left != NEG_INF else f"-{inf_symbol}"
        right_value = self.right if self.right != POS_INF else f"+{inf_symbol}"
        return f"{left_bracket}{left_value}, {right_value}{right_bracket}"


POS_INF = float("inf")
NEG_INF = float("-inf")


def seek_next(iterator: list[int], value: int | float) -> int | float:
    """
    Seek the next value in the iterator that is greater than the given value.
    If no such value exists, return POS_INF.
    Operator is idempotent, so it can be called multiple times with the same
    value.
    """
    for val in iterator:
        if val >= value:
            return val
    return POS_INF


def calculate_alternation_complexity(
    iterators: list[list[int]],
) -> list[Interval]:
    """
    Calculate the minimum partition certificate of a set of sorted lists.

    The alternation complexity δ of an instance (A₁, ..., Aₖ) is the minimum
    number of intervals that form a partition certificate. A partition
    certificate is a partition (Iⱼ)ⱼ≤δ ⊆ U such that:

    1. ∀I ∈ (Iⱼ), I = {x} ⟹ x ∈ ⋂Aᵢ  (singletons are in the intersection)
    2. ∀I ∈ (Iⱼ), |I| > 1 ⟹ ∃A ∈ (Aᵢ) : I ∩ A = ∅  (intervals certified by
    some list)

    Example from Barbay's paper (simplified):
    - Lists:
        A_1:   ·   ·   ·   ·   ·   ·   ·   ·   9   ·   ·   ·
        A_2:   1   2   ·   ·   ·   ·   ·   ·   9   ·  11   ·
        A_3:   ·   ·   3   ·   ·   ·   ·   ·   9   ·   ·  12
        A_4:   ·   ·   ·   ·   ·   ·   ·   ·   9   ·   ·   ·
        A_5:   ·   ·   ·   4   ·   ·   ·   ·   ·  10   ·   ·
        A_6:   ·   ·   ·   ·   5   6   7   ·   ·  10   ·   ·
        A_7:   ·   ·   ·   ·   ·   ·   ·   8   ·  10   ·   ·
    - Intersection: ∅ (empty)
    - Possible certificate (minimum): (-∞,9), [9,10), [10,+∞) → δ = 3

    Args:
        lists: List of lists of sorted integers.

    Returns:
        List of integers representing the partition certificate:
        - 0: element in the intersection (singleton)
        - i (i≥1): interval certified by the list i-1 (in position i)

        Example from Barbay's paper:
        - Result: [4] * 8 + [5] + [1] * 3
        - Compressed: [4, 5, 1] (using groupby)
        - δ = len([4, 5, 1]) = 3

        Note: We return the complete sequence instead of δ directly
        because it allows for more detailed analysis of the certificate and
        facilitates debugging.
        To calculate alternation complexity: len(list(groupby(resultado)))
    """

    intervals: list[Interval] = []
    left_bound, right_bound = NEG_INF, NEG_INF
    while right_bound < POS_INF:
        # Advance all iterators
        values = [seek_next(its, left_bound) for its in iterators]
        current_value = max(values)  # Greedy choice

        # Check if element it's in the intersection
        values = [seek_next(its, current_value) for its in iterators]
        if all(val == current_value for val in values) and current_value != POS_INF:
            # Add singleton interval
            intervals.append(Interval(current_value, current_value))
            left_bound, right_bound = current_value + 1, current_value + 1
        else:
            # Add interval
            right_bound = current_value
            intervals.append(Interval(left_bound, right_bound))
            left_bound = right_bound

    return intervals
