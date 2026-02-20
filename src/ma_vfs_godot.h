#pragma once

#include "miniaudio.h"

#include <godot_cpp/classes/file_access.hpp>

/*
   ma_vfs_godot contains bindings from the godot filesystem to the respective ma_vfs callbacks. namely:

   MA_API ma_result ma_vfs_open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile);
   // ma_vfs_open_w(ma_vfs* pVFS, const wchar_t* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile);
   MA_API ma_result ma_vfs_close(ma_vfs* pVFS, ma_vfs_file file);
   MA_API ma_result ma_vfs_read(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead);
   // MA_API ma_result ma_vfs_write(ma_vfs* pVFS, ma_vfs_file file, const void* pSrc, size_t sizeInBytes, size_t* pBytesWritten);
   MA_API ma_result ma_vfs_seek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin);
   MA_API ma_result ma_vfs_tell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor);
   MA_API ma_result ma_vfs_info(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo);
   // MA_API ma_result ma_vfs_open_and_read_file(ma_vfs* pVFS, const char* pFilePath, void** ppData, size_t* pSize, const ma_allocation_callbacks* pAllocationCallbacks);
   
   from miniaudio.h ^
   we ignore anything write related since this is only used for reading from audio files baked into the executable
   as well as open_and_read_file, since miniaudio will use the other functions as fallbacks
   
   ngl this code was written by gemini (3.1 Pro)
*/

inline godot::Ref<godot::FileAccess>* get_file_ptr(ma_vfs_file file) { return static_cast<godot::Ref<godot::FileAccess>*>(file); }

ma_result ma_vfs_open_godot(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
{
   if( openMode & MA_OPEN_MODE_WRITE ) return MA_NOT_IMPLEMENTED; 
   
   godot::String path = godot::String::utf8(pFilePath);
   godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::READ);
   
   if( file.is_null() ) return MA_DOES_NOT_EXIST;
   
   godot::Ref<godot::FileAccess>* file_ptr = new godot::Ref<godot::FileAccess>(file); // malloc
   
   *pFile = (ma_vfs_file)file_ptr;
   
   return MA_SUCCESS;
}

ma_result ma_vfs_close_godot(ma_vfs* pVFS, ma_vfs_file file)
{
   if( file == nullptr ) return MA_INVALID_FILE;
   godot::Ref<godot::FileAccess>* file_ptr = get_file_ptr(file);
   
   if( file_ptr->is_valid() ) (*file_ptr)->close();
   delete file_ptr; // free

   return MA_SUCCESS;
}

ma_result ma_vfs_read_godot(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
{
   if( file == nullptr ) return MA_INVALID_FILE;
   godot::Ref<godot::FileAccess>* file_ptr = get_file_ptr(file);
   
   uint64_t bytes_read = (*file_ptr)->get_buffer(static_cast<uint8_t*>(pDst), sizeInBytes);
   
   if( pBytesRead != nullptr ) *pBytesRead = bytes_read;
   if( bytes_read == 0 && sizeInBytes > 0 ) return MA_AT_END;

   return MA_SUCCESS;
}

ma_result ma_vfs_seek_godot(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
{
   if( file == nullptr ) return MA_INVALID_FILE;
   godot::Ref<godot::FileAccess>* file_ptr = get_file_ptr(file);
   
   uint64_t target_pos = 0;

   if     ( origin == ma_seek_origin_start   ) target_pos = offset;
   else if( origin == ma_seek_origin_current ) target_pos = (*file_ptr)->get_position() + offset;
   else if( origin == ma_seek_origin_end     ) target_pos = (*file_ptr)->get_length() + offset;

   (*file_ptr)->seek(target_pos);
   
   return MA_SUCCESS;
}

ma_result ma_vfs_tell_godot(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor)
{
   if( file == nullptr ) return MA_INVALID_FILE;
   godot::Ref<godot::FileAccess>* file_ptr = get_file_ptr(file);
   
   if( pCursor != nullptr ) *pCursor = (*file_ptr)->get_position();
   
   return MA_SUCCESS;
}

ma_result ma_vfs_info_godot(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo)
{
   if( file == nullptr ) return MA_INVALID_FILE;
   godot::Ref<godot::FileAccess>* file_ptr = get_file_ptr(file);
   
   if( pInfo != nullptr ) pInfo->sizeInBytes = (*file_ptr)->get_length();
   
   return MA_SUCCESS;
}

static ma_vfs_callbacks ma_vfs_callbacks_godot
{
    ma_vfs_open_godot,
    nullptr,
    ma_vfs_close_godot,
    ma_vfs_read_godot,
    nullptr,
    ma_vfs_seek_godot,
    ma_vfs_tell_godot,
    ma_vfs_info_godot
};

struct ma_vfs_godot_struct { ma_vfs_callbacks cb = ma_vfs_callbacks_godot; };