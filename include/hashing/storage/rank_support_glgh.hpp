/* sdsl - succinct data structures library
    Copyright (C) 2008-2013 Simon Gog

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/ .

    MODIFIED VERSION: Based on rank_support_v.hpp from SDSL.
    Modified to support on-the-fly bitvector computation from Gl and Gh bitvectors
    for the GlGhStorage MPHF strategy. The bitvector B is computed as B[v] = ~(Gl[v] & Gh[v])
    on-the-fly instead of being stored explicitly.
*/
/*! \file rank_support_glgh.hpp
    \brief rank_support_glgh.hpp contains rank_support_glgh (modified from rank_support_v).
    \author Based on work by Simon Gog, modified for on-the-fly bitvector computation
*/
// clang-format off
#ifndef INCLUDED_CLTJ_RANK_SUPPORT_GLGH
#define INCLUDED_CLTJ_RANK_SUPPORT_GLGH

#include <sdsl/rank_support.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/bits.hpp>

using namespace sdsl;

//! Namespace for the compactLTJ library.
namespace cltj {
namespace hashing {


//! A rank structure for on-the-fly bitvector computation (based on rank_support_v by Sebastiano Vigna)
/*! \par Space complexity
 *  \f$ 0.25n\f$ for a bit vector of length n bits.
 *
 * The superblock size is 512. Each superblock is subdivided into 512/64 = 8
 * blocks. So absolute counts for the superblock add 64/512 bits on top of each
 * supported bit. Since the first of the 8 relative count values is 0, we can
 * fit the remaining 7 (each of width log(512)=9) in a 64bit word. The relative
 * counts add another 64/512 bits on top of each supported bit.
 * In total this results in 128/512=25% overhead.
 *
 * This modified version computes the bitvector B on-the-fly from Gl and Gh:
 * B[v] = ~(Gl[v] & Gh[v])
 *
 * \tparam t_b       Bit pattern `0`,`1`,`10`,`01` which should be ranked.
 * \tparam t_pat_len Length of the bit pattern.
 *
 * \par Reference
 *    Sebastiano Vigna:
 *    Broadword Implementation of Rank/Select Queries.
 *    WEA 2008: 154-168
 */
template<uint8_t t_b=1, uint8_t t_pat_len=1>
class rank_support_glgh : public rank_support
{
    private:
        // For the GlGh MPHF storage we only need rank1 support.
        static_assert(t_b == 1u && t_pat_len == 1u,
                      "rank_support_glgh only supports rank1 (t_b=1, t_pat_len=1)");
    public:
        typedef bit_vector                          bit_vector_type;
        typedef rank_support_trait<t_b, t_pat_len>  trait_type;
        enum { bit_pat = t_b };
        enum { bit_pat_len = t_pat_len };
    private:
        // basic block for interleaved storage of superblockrank and blockrank
        int_vector<64> m_basic_block;
        // Pointers to Gl and Gh bitvectors from which B is computed on-the-fly.
        const bit_vector* m_gl = nullptr;
        const bit_vector* m_gh = nullptr;
    public:
        explicit rank_support_glgh(const bit_vector* gl = nullptr,
                                   const bit_vector* gh = nullptr) {
            // m_v (from base) will point to Gl; we never store B explicitly.
            m_gl = gl;
            m_gh = gh;
            m_v  = gl;

            if (gl == nullptr || gh == nullptr) {
                return;
            } else if (gl->empty()) {
                m_basic_block = int_vector<64>(2,0);   // resize structure for basic_blocks
                return;
            }
            size_type basic_block_size = ((gl->capacity() >> 9)+1)<<1;
            m_basic_block.resize(basic_block_size);   // resize structure for basic_blocks
            if (m_basic_block.empty())
                return;
            const uint64_t* gl_data = m_gl->data();
            const uint64_t* gh_data = m_gh->data();
            size_type i, j=0;
            m_basic_block[0] = m_basic_block[1] = 0;

            // First word of B: B_word = ~(Gl_word & Gh_word).
            uint64_t first_gl = *gl_data;
            uint64_t first_gh = *gh_data;
            uint64_t b_word   = ~(first_gl & first_gh);
            uint64_t sum      = sdsl::bits::cnt(b_word);
            uint64_t second_level_cnt = 0;
            for (i = 1; i < (m_gl->capacity()>>6) ; ++i) {
                uint64_t word_gl = gl_data[i];
                uint64_t word_gh = gh_data[i];
                b_word = ~(word_gl & word_gh);
                if (!(i&0x7)) {// if i%8==0
                    j += 2;
                    m_basic_block[j-1] = second_level_cnt;
                    m_basic_block[j] 	= m_basic_block[j-2] + sum;
                    second_level_cnt = sum = 0;
                } else {
                    second_level_cnt |= sum<<(63-9*(i&0x7));//  54, 45, 36, 27, 18, 9, 0
                }
                sum += sdsl::bits::cnt(b_word);
            }
            if (i&0x7) { // if i%8 != 0
                second_level_cnt |= sum << (63-9*(i&0x7));
                m_basic_block[j+1] = second_level_cnt;
            } else { // if i%8 == 0
                j += 2;
                m_basic_block[j-1] = second_level_cnt;
                m_basic_block[j]   = m_basic_block[j-2] + sum;
                m_basic_block[j+1] = 0;
            }
        }

        rank_support_glgh(const rank_support_glgh&)  = default;
        rank_support_glgh(rank_support_glgh&&) = default;
        rank_support_glgh& operator=(const rank_support_glgh&) = default;
        rank_support_glgh& operator=(rank_support_glgh&&) = default;


        size_type rank(size_type idx) const {
            assert(m_gl != nullptr && m_gh != nullptr);
            assert(idx <= m_gl->size());
            const uint64_t* p = m_basic_block.data()
                                + ((idx>>8)&0xFFFFFFFFFFFFFFFEULL); // (idx/512)*2
            size_type result = *p + ((*(p+1)>>(63 - 9*((idx&0x1FF)>>6)))&0x1FF);
            if (idx&0x3F) { // if (idx%64)!=0
                // Add contribution of the remaining bits in the current 64-bit word.
                size_type word_idx = idx >> 6;
                uint8_t  offset    = idx & 0x3F;
                const uint64_t* gl_data = m_gl->data();
                const uint64_t* gh_data = m_gh->data();
                uint64_t b_word = ~(gl_data[word_idx] & gh_data[word_idx]);
                uint64_t mask   = sdsl::bits::lo_set[offset];
                result += sdsl::bits::cnt(b_word & mask);
            }
            return result;
        }

        inline size_type operator()(size_type idx)const {
            return rank(idx);
        }

        size_type size()const {
            return m_gl ? m_gl->size() : 0;
        }

        size_type serialize(std::ostream& out, structure_tree_node* v=nullptr,
                            std::string name="")const {
            size_type written_bytes = 0;
            structure_tree_node* child = structure_tree::add_child(v, name,
                                         util::class_name(*this));
            written_bytes += m_basic_block.serialize(out, child,
                             "cumulative_counts");
            structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream& in, const int_vector<1>* v=nullptr) {
            set_vector(v);
            m_basic_block.load(in);
        }

        void set_vector(const bit_vector* v=nullptr) override {
            // For compatibility with the base interface, treat v as Gl.
            m_gl = v;
            m_v  = v;
        }

        void swap(rank_support_glgh& rs) {
            if (this != &rs) { // if rs and _this_ are not the same object
                m_basic_block.swap(rs.m_basic_block);
            }
        }
};

}  // namespace hashing
}  // namespace cltj

// clang-format on
#endif  // end file
