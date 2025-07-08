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
        List of intervals (start, end) representing the partition certificate.
        Each interval can be:
        - Singleton: (x, x) where x is in the intersection ⋂Aᵢ
        - Non-singleton: (a, b) where a < b, certified by some list that
          has empty intersection with the interval

        Example from Barbay's paper:
        - Result: [(-∞, 9), (9, 10), (10, +∞)]
        - δ = len(result) = 3


        To calculate alternation complexity: len(result)
    """

    intervals: list[Interval] = []
    left_bound, right_bound = NEG_INF, NEG_INF
    previous_value_in_intersection = False
    while right_bound < POS_INF:
        # Advance all iterators
        value_to_seek = left_bound
        if previous_value_in_intersection:
            value_to_seek += 1
        values = [seek_next(its, value_to_seek) for its in iterators]
        current_value = max(values)  # Greedy choice

        # Check if element it's in the intersection
        values = [seek_next(its, current_value) for its in iterators]
        if all(val == current_value for val in values) and current_value != POS_INF:
            # Add gap before singleton and singleton itself
            intervals.append(Interval(left_bound, current_value))
            intervals.append(Interval(current_value, current_value))
            left_bound = current_value
            previous_value_in_intersection = True
        else:
            # Add interval
            right_bound = current_value
            intervals.append(Interval(left_bound, right_bound))
            left_bound = right_bound
            previous_value_in_intersection = False

    return intervals
