/**
 * @file data_structure.h
 * @author Zhao Jia
 * @brief define the data structure in deduplication
 * @version 0.1
 * @date 2021-8
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef EDRSTORE_DATA_STRUCTURE_H
#define EDRSTORE_DATA_STRUCTURE_H

#include "const_var.h"
#include <bits/stdc++.h>

typedef struct {
    uint32_t size;
    uint8_t data[MAX_CHUNK_SIZE];
    uint8_t fp[CHUNK_HASH_SIZE];
} RawChunk_t;

typedef struct {
    uint64_t size;
    uint64_t chunk_num;
} FileRecipeHead_t;

typedef struct {
    union {
        RawChunk_t raw_chunk;
        FileRecipeHead_t head;
    };
    int type;
} Chunk_t;

typedef Chunk_t Chunker2KeyGen_t;

typedef struct {
    Chunk_t chunk;
    uint64_t features[SUPER_FEATURE_PER_CHUNK];    
} FeatureChunk_t;

typedef struct {
    FeatureChunk_t feature_chunk;
    uint8_t enc_data[ENC_MAX_CHUNK_SIZE];
    uint32_t enc_size;
    uint8_t key[CHUNK_HASH_SIZE];
    uint64_t seed;
} EncFeatureChunk_t;

typedef EncFeatureChunk_t KeyGen2SelectComp_t;

typedef struct {
    uint32_t size;
    uint8_t type;
    uint64_t cipher_features[SUPER_FEATURE_PER_CHUNK];
} SendChunkHeader_t;

typedef struct {
    SendChunkHeader_t header;
    uint8_t data[ENC_MAX_CHUNK_SIZE];
} SendChunk_t;

typedef struct {
    uint8_t key[CHUNK_HASH_SIZE];
} KeyRecipe_t;

typedef struct {
    SendChunk_t send_chunk; 
    KeyRecipe_t key_recipe;
} SelectComp2Sender_t;

typedef struct {
    uint8_t key[CHUNK_HASH_SIZE];
    uint64_t seed;
} KeyGenRet_t;

typedef struct {
    uint8_t fp[CHUNK_HASH_SIZE];
    uint64_t features[SUPER_FEATURE_PER_CHUNK];
} KeyGenReq_t;

typedef struct {
    uint8_t hash[CHUNK_HASH_SIZE];
    uint8_t key[CHUNK_HASH_SIZE];
} RecipeEntry_t;

typedef struct {
    uint8_t stat;
    uint8_t container_id[CONTAINER_ID_LENGTH];
    uint32_t offset;
    uint32_t len;
    uint8_t base_fp[CHUNK_HASH_SIZE];
} KeyForChunkHashDB_t;

typedef struct {
    char id[CONTAINER_ID_LENGTH];
    uint8_t body[MAX_CONTAINER_SIZE];
    uint32_t cur_size;
} Container_t;

typedef struct {
    uint32_t client_id;
    uint32_t msg_type;
    uint32_t cur_item_num;
    uint32_t size;
} NetworkHead_t;

typedef struct {
    NetworkHead_t* header;
    uint8_t* send_buf;
    uint8_t* data_buf;
} SendMsgBuffer_t;

typedef struct {
    uint8_t fp[CHUNK_HASH_SIZE]; // chunk fp
    uint64_t features[SUPER_FEATURE_PER_CHUNK];
    KeyForChunkHashDB_t addr;
    uint32_t size;
    uint8_t stat;
} ChunkInfo_t;

typedef struct {
    ChunkInfo_t info;
    uint8_t data[ENC_MAX_CHUNK_SIZE];
} WrappedChunk_t;

typedef struct {
    uint8_t* buf;
    uint32_t cnt;
} BatchBuf_t;

typedef struct {
    SendChunk_t input_chunk;
    SendChunk_t base_chunk;
} Reader2Decoder_t;

typedef SendChunk_t Retriever2Writer_t;

#endif