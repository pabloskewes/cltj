format:
	@echo "Formatting code..."
	@find . -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -style=file -i {} \;
	@echo "Done."