from dataclasses import dataclass


@dataclass(frozen=True)
class Interval:
    """
    Represents an interval in the universe of integers.

    Attributes:
        start: The start of the interval.
        end: The end of the interval.
        belongs: Whether the interval belongs to the list.
    """

    start: int
    end: int
    belongs: bool

    def __str__(self):
        if self.start == self.end:
            return f"[{self.start}]" if self.belongs else f"({self.start})"
        return (
            f"[{self.start}, {self.end}]"
            if self.belongs
            else f"({self.start}-{self.end})"
        )

    def __repr__(self):
        return self.__str__()


def preprocess_lists(lists: list[list[int]]) -> list[list[Interval]]:
    """
    Compress lists of sorted integers into a compressed interval representation.

    This function takes k lists of sorted integers and converts them into a
    compressed representation where each list is represented as a sequence of
    intervals that alternate between elements that belong to the list and gaps
    (empty spaces).

    Args:
        lists: List of k lists of sorted integers.
               Example: [[1, 3, 5], [2, 3], [4, 5, 6]]

    Returns:
        List where result[i] contains the compressed intervals of lists[i].
        Each interval is an object Interval(start, end, belongs) where:
        - start, end: range of the interval [start, end] (inclusive)
        - belongs: True if the elements belong to the list, False if it's a gap

    Example:
        Input: [[1, 3, 5], [2, 3], [4, 5, 6]]
        Universe: [1, 2, 3, 4, 5, 6]
        Output: [
            [[1], (2), [3], (4), [5], (6)],
            [(1), [2], [3], (4-6)],
            [(1-3), [4], [5], [6]]
        ]
        Visual representation:
        A_1:   1   ·   3   ·   5   ·
        A_2:   ·   2   3   ·   ·   ·
        A_3:   ·   ·   ·   4   5   6

    Complexity:
        Time: O(n log n) where n = Σ|lists[i]|
        Space: O(n)

    """
    if not lists or all(len(lst) == 0 for lst in lists):
        return []

    # Find the complete universe
    all_values = set()
    for lst in lists:
        all_values.update(lst)

    if not all_values:
        return []

    min_val = min(all_values)
    max_val = max(all_values)

    result = []

    for lst in lists:
        if not lst:
            # Empty list: the whole universe is a gap
            result.append([Interval(min_val, max_val, False)])
            continue

        intervals: list[Interval] = []
        sorted_lst = sorted(lst)  # Just in case it's not sorted

        # Initial gap (if exists)
        if sorted_lst[0] > min_val:
            intervals.append(
                Interval(
                    start=min_val,
                    end=sorted_lst[0] - 1,
                    belongs=False,
                )
            )

        # Process elements and intermediate gaps
        start: int | None = None
        for i, val in enumerate(sorted_lst):
            # Add the element (it may be part of a consecutive range)
            if i == 0 or val > sorted_lst[i - 1] + 1:
                # Start of new consecutive range
                start = val

            # If it's the last element or there's a gap after
            if i == len(sorted_lst) - 1 or val + 1 < sorted_lst[i + 1]:
                if start is None:
                    raise ValueError("Start is None")

                # End of consecutive range
                intervals.append(
                    Interval(
                        start=start,
                        end=val,
                        belongs=True,
                    )
                )

                # Gap after (if not the last element)
                if i < len(sorted_lst) - 1:
                    intervals.append(
                        Interval(
                            start=val + 1,
                            end=sorted_lst[i + 1] - 1,
                            belongs=False,
                        )
                    )

        # Final gap (if exists)
        if sorted_lst[-1] < max_val:
            intervals.append(
                Interval(
                    start=sorted_lst[-1] + 1,
                    end=max_val,
                    belongs=False,
                )
            )

        result.append(intervals)

    return result
