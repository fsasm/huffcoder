/*
 * @file huff_enc.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-20
 */

#include <stdlib.h>
#include <assert.h>
#include "huff_enc.h"

struct node {
	struct node *left;
	struct node *right;
	struct huff_code *code;
	uint32_t count;
};

void huff_get_freq(const uint8_t data[restrict], size_t size, 
					   uint32_t freq[restrict 256])
{
	if (size == 0)
		return;
	
	for (int i = 0; i < 256; i++)
		freq[i] = 0;

	for (size_t i = 0; i < size; i++) {
		uint8_t symbol = data[i];
		freq[symbol]++;
	}
}

static bool gen_code_lengths(uint16_t num_sym, const uint32_t freq[restrict],
							 struct huff_code codes[restrict]);
static void gen_canonical_codes(uint16_t num_codes, 
								struct huff_code codes[restrict], 
								struct huff_enc_info * restrict info);

bool huff_gen_enc(const uint32_t freq[restrict 256],
				  struct huff_enc * restrict encoder, 
				  struct huff_enc_info * restrict info)
{
	assert(freq    != NULL);
	assert(encoder != NULL);
	assert(info    != NULL);

	uint16_t num_sym = 0;

	for (int i = 0; i < 256; i++) {
		if (freq[i] != 0)
			num_sym++;
	}

	if (num_sym == 0) {
		fprintf(stderr, "Invalid number of symbols\n");
		return false;
	}

	struct huff_code *codes = malloc(num_sym * sizeof(*codes));
	if (codes == NULL) {
		perror("Couldn't allocate code table");
		return false;
	}

	/* generate huffman code lengths */
	gen_code_lengths(num_sym, freq, codes);
	//limit_length(num_sym, codes);

	/* generate canonical huffman codes */
	gen_canonical_codes(num_sym, codes, info);

	encoder->num_codes = num_sym;
	encoder->codes = codes;
	info->num_codes = num_sym;

	return true;
}

void huff_enc_destroy(struct huff_enc *encoder)
{
	free(encoder->codes);
}

bool huff_encode(const struct huff_enc * restrict encoder, size_t num_sym,
				 const uint8_t in_data[restrict], 
				 struct bit_writer * restrict writer)
{
	struct huff_code *codes = encoder->codes;
	for (size_t i = 0; i < num_sym; i++) {
		uint8_t symbol = in_data[i];
		uint16_t code = 0;
		uint8_t code_len = 0;

		for (uint16_t j = 0; j < encoder->num_codes; j++) {
			if (symbol != codes[j].symbol)
				continue;

			code = codes[j].code;
			code_len = codes[j].code_len;
			break;
		}

		assert(0 < code_len && code_len <= 16);

		bit_writer_next_bits(writer, code, code_len);
	}

	return true;
}

static void get_min_nodes(uint16_t n, const struct node nodes[restrict],
						  uint16_t * restrict min1_index, 
						  uint16_t * restrict min2_index)
{
	assert(n >= 2);
	assert(nodes != NULL);
	assert(min1_index != NULL);
	assert(min2_index != NULL);

	uint16_t index1 = 0;
	uint16_t index2 = 0;

	uint32_t count1 = nodes[0].count;
	for (uint16_t i = 1; i < n; i++) {
		if (count1 > nodes[i].count) {
			count1 = nodes[i].count;
			index1 = i;
		}
	}
	
	uint32_t count2;
	if (index1 == 0) {
		count2 = nodes[1].count;
		index2 = 1;
	} else {
		count2 = nodes[0].count;
		index2 = 0;
	}

	for (uint16_t i = 1; i < n; i++) {
		if (count2 > nodes[i].count && i != index1) {
			count2 = nodes[i].count;
			index2 = i;
		}
	}

	assert(index1 != index2);
	assert(index1 < n);
	assert(index2 < n);

	*min1_index = index1;
	*min2_index = index2;
}

static void set_len(const struct node *node, uint8_t depth)
{
	assert(node != NULL);

	if (depth > 16) {
		fprintf(stderr, "Warning: Code length over 16!\n");
	}
	if (node->left == NULL && node->right == NULL) {
		assert(node->code != NULL);
		node->code->code_len = depth;
	} else {
		assert(node->code  == NULL);
		assert(node->left  != NULL);
		assert(node->right != NULL);

		set_len(node->left,  depth + 1);
		set_len(node->right, depth + 1);
	}
}

static bool gen_code_lengths(uint16_t num_sym, const uint32_t freq[restrict],
							 struct huff_code codes[restrict])
{
	assert(0 < num_sym && num_sym <= 256);
	assert(freq  != NULL);
	assert(codes != NULL);

	struct node *nodes = malloc((2 * num_sym - 1) * sizeof(struct node));
	size_t node_index = 0;
	uint32_t freq_sum = 0;
	
	for (int i = 0; i < 256; i++) {
		if (freq[i] == 0)
			continue;

		freq_sum += freq[i];

		codes[node_index].symbol = i;
		nodes[node_index] = (struct node) {
			.left = NULL,
			.right = NULL,
			.code = &codes[node_index],
			.count = freq[i]
		};
		node_index++;
	}

	assert(0 < node_index && node_index <= 256);

	uint16_t border = num_sym;
	uint16_t last   = 2 * num_sym - 2;

	while (border > 2) {
		uint16_t min1_index;
		uint16_t min2_index;

		get_min_nodes(border, nodes, &min1_index, &min2_index);
		assert(min1_index < border && min2_index < border);
		assert(nodes[min1_index].count <= nodes[min2_index].count);

		nodes[last]     = nodes[min2_index];
		nodes[last - 1] = nodes[min1_index];
		
		nodes[min1_index].count += nodes[min2_index].count;
		nodes[min1_index].left  = &nodes[last - 1];
		nodes[min1_index].right = &nodes[last];
		nodes[min1_index].code  = NULL;
		
		nodes[min2_index] = nodes[border - 1];

		border--;
		last -= 2;
	}

	if (nodes[0].count > nodes[1].count) {
		struct node tmp = nodes[1];
		nodes[1] = nodes[0];
		nodes[0] = tmp;
	}

	assert(nodes[0].count <= nodes[1].count);

	struct node root = (struct node) {
		.left  = &nodes[0],
		.right = &nodes[1],
		.code  = NULL,
		.count = nodes[0].count + nodes[1].count
	};

	assert(root.count == freq_sum);

	set_len(&root, 0);
	free(nodes);

	return true;
}

static void gen_canonical_codes(uint16_t num_codes, 
								struct huff_code codes[restrict], 
								struct huff_enc_info * restrict info) {
	uint16_t code = 0;
	uint8_t min_bits = 0;
	uint8_t max_bits = 0;

	for (int i = 0; i < 16; i++) {
		uint8_t num_codes_len = 0;
		for (int j = 0; j < num_codes; j++) {
			assert(codes[j].code_len <= 16);
			if (codes[j].code_len != i + 1)
				continue;

			codes[j].code = code;
			code++;
			num_codes_len++;
		}

		code <<= 1;

		if (num_codes_len > 0)
			max_bits = i + 1;

		if (min_bits == 0 && num_codes_len > 0)
			min_bits = i + 1;

		info->codes_per_len[i] = num_codes_len;
	}

	info->min_bits = min_bits;
	info->max_bits = max_bits;
}

