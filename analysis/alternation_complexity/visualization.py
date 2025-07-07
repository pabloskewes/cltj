def plot_lists_visual(lists, list_names=None, header=True, show_dots=False):
    """
    Visualize the lists in a grid format where each value is placed in its
    corresponding column to facilitate viewing intersections.

    Args:
        lists: List of sorted lists
        list_names: Optional names for the lists (e.g., ['A', 'B', 'C'])
        header: Whether to display the header with the values as columns
        show_dots: Whether to show small dots (·) in empty spaces
    """
    if not lists:
        print("No lists to show")
        return

    # If no names are provided, use A_1, A_2, etc.
    if list_names is None:
        list_names = [f"A_{i+1}" for i in range(len(lists))]

    # Find the range of values to determine the columns
    all_values = []
    for lst in lists:
        all_values.extend(lst)

    if not all_values:
        print("All lists are empty")
        return

    min_val = min(all_values)
    max_val = max(all_values)

    if header:
        # Create header with the values as columns
        print("Lista:", end="")
        for val in range(min_val, max_val + 1):
            print(f"{val:>4}", end="")
        print()

    # Show each list
    for i, (lst, name) in enumerate(zip(lists, list_names)):
        print(f"{name:>5}:", end="")

        # For each possible value, show if it is in the list
        for val in range(min_val, max_val + 1):
            if val in lst:
                print(f"{val:>4}", end="")
            else:
                if show_dots:
                    print("   ·", end="")  # 3 spaces + dot
                else:
                    print("    ", end="")  # 4 spaces
        print()

    print()
