// Test suite for TritPackedArray
#include <hashing/storage/trit_packed_array.hpp>
#include <cassert>
#include <iostream>
#include <sstream>

using cltj::hashing::TritPackedArray;

/**
 * Test básico: inicialización y valores por defecto
 */
void test_initialization() {
    std::cout << "Testing initialization... ";
    TritPackedArray arr;
    arr.initialize(5);

    assert(arr.size() == 5 && "Size should be 5");

    // Todos los trits deben inicializarse en 0
    for (uint32_t i = 0; i < 5; i++) {
        assert(arr.get(i) == 0 && "Trit should be 0 after initialization");
    }
    std::cout << "PASSED\n";
}

/**
 * Test: escritura y lectura básica
 */
void test_basic_set_get() {
    std::cout << "Testing basic set/get... ";
    TritPackedArray arr;
    arr.initialize(5);

    // Escribe secuencia [2, 1, 0, 2, 1]
    arr.set(0, 2);
    arr.set(1, 1);
    arr.set(2, 0);
    arr.set(3, 2);
    arr.set(4, 1);

    // Lee y verifica
    assert(arr.get(0) == 2 && "Trit at index 0 should be 2");
    assert(arr.get(1) == 1 && "Trit at index 1 should be 1");
    assert(arr.get(2) == 0 && "Trit at index 2 should be 0");
    assert(arr.get(3) == 2 && "Trit at index 3 should be 2");
    assert(arr.get(4) == 1 && "Trit at index 4 should be 1");
    std::cout << "PASSED\n";
}

/**
 * Test: modificación de valores existentes
 */
void test_modification() {
    std::cout << "Testing modification... ";
    TritPackedArray arr;
    arr.initialize(3);

    arr.set(0, 1);
    arr.set(1, 2);
    arr.set(2, 0);

    assert(arr.get(0) == 1 && "Trit at index 0 should be 1");
    assert(arr.get(1) == 2 && "Trit at index 1 should be 2");
    assert(arr.get(2) == 0 && "Trit at index 2 should be 0");

    // Modifica valores
    arr.set(0, 0);
    arr.set(1, 1);
    arr.set(2, 2);

    assert(arr.get(0) == 0 && "Trit at index 0 should be 0 after modification");
    assert(arr.get(1) == 1 && "Trit at index 1 should be 1 after modification");
    assert(arr.get(2) == 2 && "Trit at index 2 should be 2 after modification");
    std::cout << "PASSED\n";
}

/**
 * Test: múltiples words (más de 20 trits)
 */
void test_multiple_words() {
    std::cout << "Testing multiple words... ";
    TritPackedArray arr;
    arr.initialize(50);  // 3 words necesarios (20 + 20 + 10)

    // Escribe patrón: índice % 3
    for (uint32_t i = 0; i < 50; i++) {
        arr.set(i, i % 3);
    }

    // Verifica patrón
    for (uint32_t i = 0; i < 50; i++) {
        assert(arr.get(i) == (i % 3) && "Pattern verification failed");
    }
    std::cout << "PASSED\n";
}

/**
 * Test: valores en el borde de word (posiciones 19-20-21)
 */
void test_word_boundaries() {
    std::cout << "Testing word boundaries... ";
    TritPackedArray arr;
    arr.initialize(40);

    // Escribe valores específicos en bordes de words
    arr.set(19, 2);  // Último trit del primer word
    arr.set(20, 1);  // Primer trit del segundo word
    arr.set(21, 0);  // Segundo trit del segundo word

    assert(arr.get(19) == 2 && "Trit at boundary 19 should be 2");
    assert(arr.get(20) == 1 && "Trit at boundary 20 should be 1");
    assert(arr.get(21) == 0 && "Trit at boundary 21 should be 0");
    std::cout << "PASSED\n";
}

/**
 * Test: serialización y carga
 */
void test_serialization() {
    std::cout << "Testing serialization... ";
    TritPackedArray arr1;
    arr1.initialize(10);

    // Escribe datos
    for (uint32_t i = 0; i < 10; i++) {
        arr1.set(i, (i * 2) % 3);
    }

    // Serializa
    std::stringstream ss;
    arr1.serialize(ss, nullptr, "test");

    // Carga en nuevo array
    TritPackedArray arr2;
    arr2.load(ss);

    // Verifica que sean iguales
    assert(arr2.size() == arr1.size() && "Size mismatch after load");
    for (uint32_t i = 0; i < arr1.size(); i++) {
        assert(arr2.get(i) == arr1.get(i) && "Value mismatch after load");
    }
    std::cout << "PASSED\n";
}

/**
 * Test: tamaño en bytes
 */
void test_size_in_bytes() {
    std::cout << "Testing size_in_bytes... ";
    TritPackedArray arr;
    arr.initialize(100);  // 5 words necesarios

    size_t expected_size = sizeof(uint32_t) +  // n_
        5 * sizeof(uint32_t);  // 5 words

    assert(arr.size_in_bytes() == expected_size && "Size calculation incorrect");
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== TritPackedArray Tests ===\n\n";

    try {
        test_initialization();
        test_basic_set_get();
        test_modification();
        test_multiple_words();
        test_word_boundaries();
        test_serialization();
        test_size_in_bytes();

        std::cout << "\n✅ All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << "\n";
        return 1;
    }
}
