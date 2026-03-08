#include "gctoolsplusplus/Archive.hpp"
#include "gctoolsplusplus/Util.hpp"
#include "bstream.h"
#include <algorithm>
#include <map>
#include <utility>
#include <functional>
#include <unordered_set>

namespace Archive {

uint16_t Hash(std::string str){
    uint16_t hash = 0;

    for (std::size_t i = 0; i < str.size(); i++){
        hash = (uint16_t)(hash * 3);
        hash += (uint16_t)str[i];
        hash &= 0xFFFF;
    }

    return hash;
}

///
/// Folder
///

Folder::Folder(std::shared_ptr<Rarc> archive){
    mArchive = archive;
}
Folder::Folder(std::shared_ptr<U8> archive) {
    mArchiveU8 = archive;
}

std::shared_ptr<File> Folder::GetFile(std::filesystem::path path) {
    if(path.begin() == path.end()) return nullptr;


    for(auto file : mFiles){
        if(file->GetName() == path.begin()->string()){
            if(((++path.begin()) == path.end())){
                return file;
            } else {
                if(file->MountAsArchive()){
                    std::filesystem::path subPath;
                    for(auto it = (++path.begin()); it != path.end(); it++) subPath  = subPath / it->string();
                    return (*file)->GetFile(subPath);
                }
            }
        }
    }

    std::shared_ptr<File> file = nullptr;

    for(auto dir : mFolders){
        if(dir->GetName() == path.begin()->string()){
            std::filesystem::path subPath;
            for(auto it = (++path.begin()); it != path.end(); it++) subPath  = subPath / it->string();
            file = dir->GetFile(subPath);
        }
    }

    return file;
}

std::shared_ptr<Folder> Folder::GetFolder(std::filesystem::path path){
    for(auto dir : mFolders){
        if(dir->GetName() == path.begin()->string() && ((++path.begin()) == path.end())){
            return dir;
        } else if(dir->GetName() == path.begin()->string()) {
            std::filesystem::path subPath;
            for(auto it = (++path.begin()); it != path.end(); it++) subPath  = subPath / it->string();
            return dir->GetFolder(subPath);
        }
    }

    return nullptr;
}

std::shared_ptr<Folder> Folder::Copy(std::shared_ptr<Rarc> archive){
    std::shared_ptr<Folder> newFolder = Folder::Create(archive);
    newFolder->SetName(mName);
    for(auto dir : mFolders){
        newFolder->AddSubdirectory(dir);
    }

    for(auto file : mFiles){
        newFolder->AddFile(file);
    }
    return newFolder;
}

std::shared_ptr<Folder> Folder::CopyU8(std::shared_ptr<U8> archive) {
    std::shared_ptr<Folder> newFolder = Folder::CreateU8(archive);
    newFolder->SetName(mName);
    for (auto dir : mFolders) {
        newFolder->AddSubdirectory(dir);
    }

    for (auto file : mFiles) {
        newFolder->AddFile(file);
    }
    return newFolder;
}

void Folder::AddSubdirectory(std::shared_ptr<Folder> dir){
    if(dir->GetArchive().lock() != mArchive.lock()){
        std::shared_ptr<Folder> copy = dir->Copy(mArchive.lock());
        AddSubdirectory(copy);
    } else {
        dir->SetParentUnsafe(GetPtr());
        mFolders.push_back(dir);

        auto dirIter = std::find(mArchive.lock()->mDirectories.begin(), mArchive.lock()->mDirectories.end(), dir);
        if(dirIter == mArchive.lock()->mDirectories.end()){
            mArchive.lock()->mDirectories.push_back(dir);
        }
    }
}

void Folder::AddSubdirectoryU8(std::shared_ptr<Folder> dir) {
    if (dir.get() == this) return;
    if (dir->GetArchive().lock() != mArchive.lock()) {
        std::shared_ptr<Folder> copy = dir->CopyU8(mArchiveU8.lock());
        AddSubdirectory(copy);
    }
    else {
        dir->SetParentUnsafe(GetPtr());
        mFolders.push_back(dir);
    }
}

///
/// File
///

bool File::MountAsArchive(){
    std::shared_ptr<Rarc> arc =  Rarc::Create();

    // There needs to be some way to specify endian here, maybe check magic and load it smart?
    bStream::CMemoryStream stream(mData, mSize, bStream::Endianess::Big, bStream::OpenMode::In);

    if(arc->Load(&stream)){
        mMountedArchive = arc;
        return true;
    } else {
        mMountedArchive = nullptr;
        return false;
    }

}


///
/// Archive
///

std::map<std::string, uint32_t> Rarc::CalculateArchiveSizes(){
    uint32_t size = 0x40;

    std::map<std::string, bool> isUniqueStr;

    uint32_t dirEntrySize = 0x00; //??
    uint32_t fileEntrySize = 0;
    uint32_t fileDataSize = 0;
    uint32_t strTableSize = 5; // Accounts for .\0 and ..\0 strs

    for(auto dir : mDirectories){
        dirEntrySize += 0x10;

        if(isUniqueStr.count(dir->GetName()) == 0){
            strTableSize += dir->GetName().size() + 1;
            isUniqueStr.insert({dir->GetName(), true});
        }

        fileEntrySize += 0x14 + 0x14; // . and .. entries

        for(auto subdir : dir->GetSubdirectories()){
            fileEntrySize += 0x14;
        }

        for(auto file : dir->GetFiles()){
            if(isUniqueStr.count(file->GetName()) == 0){
                strTableSize += file->GetName().size() + 1;
                isUniqueStr.insert({file->GetName(), true});
            }

            fileEntrySize += 0x14;
            fileDataSize += Util::PadTo32(file->GetSize());
        }
    }

    size += Util::PadTo32(dirEntrySize) + Util::PadTo32(fileEntrySize) + fileDataSize + Util::PadTo32(strTableSize);

    return {{"total", Util::PadTo32(size)}, {"dirEntries", Util::PadTo32(dirEntrySize)}, {"fileEntries", Util::PadTo32(fileEntrySize)}, {"fileData", fileDataSize}, {"strTable", Util::PadTo32(strTableSize)}};
}


void Rarc::SaveToFile(std::filesystem::path path, Compression::Format compression, uint8_t compressionLevel, bool padCompressed){

    std::map<std::string, uint32_t> archiveSizes = CalculateArchiveSizes();


    uint8_t* archiveData = new uint8_t[archiveSizes["total"]];
    memset(archiveData, 0, archiveSizes["total"]);

    uint8_t* fileSystemChunk = archiveData + 0x20;
    uint8_t* dirChunk = archiveData + 0x40;
    uint8_t* fileChunk = archiveData + 0x40 + archiveSizes["dirEntries"];
    uint8_t* strTableChunk = archiveData + 0x40 + archiveSizes["dirEntries"] + archiveSizes["fileEntries"];
    uint8_t* fileDataChunk = archiveData + 0x40 + archiveSizes["dirEntries"] + archiveSizes["fileEntries"] + archiveSizes["strTable"];

    bStream::CMemoryStream headerStream(archiveData, 0x20, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream fileSystemStream(fileSystemChunk, 0x20, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream dirStream(dirChunk, archiveSizes["dirEntries"], mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream fileStream(fileChunk, archiveSizes["fileEntries"], mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream stringTableStream(strTableChunk, archiveSizes["strTable"], mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream fileDataStream(fileDataChunk, archiveSizes["fileData"], mArchiveOrder, bStream::OpenMode::Out);

    // Generate String Table

    std::map<std::string, uint32_t> stringTable{{".", 0x00}, {"..", 0x02}};

    stringTableStream.writeString(".");
    stringTableStream.writeUInt8(0x00);
    stringTableStream.writeString("..");
    stringTableStream.writeUInt8(0x00);

    for(auto dir : mDirectories){
        if(stringTable.count(dir->GetName()) == 0){
            stringTable.insert({dir->GetName(), stringTableStream.tell()});
            stringTableStream.writeString(dir->GetName());
            stringTableStream.writeUInt8(0x00);
        }
        for(auto file : dir->GetFiles()){
            if(stringTable.count(file->GetName()) == 0){
                stringTable.insert({file->GetName(), stringTableStream.tell()});
                stringTableStream.writeString(file->GetName());
                stringTableStream.writeUInt8(0x00);
            }
        }
    }

    std::size_t currentFileIndex = 0;

    // Write Archive Structure
    for(std::size_t i = 0; i < mDirectories.size(); i++)
    {
        std::shared_ptr<Folder> folder = mDirectories[i];

        std::string folderName = folder->GetName();

        //Write IDs
        if(i == 0){
            dirStream.writeUInt32(0x524F4F54);
        } else {
            char temp[4];
            memset(temp, 0x20, sizeof(temp));
            for (std::size_t ch = 0; ch < 4; ch++){
                if(ch >= folderName.size()) break;
                temp[ch] = toupper(folderName[ch]);
            }
            // This is a hack, these are technically ints but if we treat them that way the expected IDs end up broken in BE exports.
            // There is probably a better way to do this but im too lazy to fix it right now
            if(dirStream.getOrder() == bStream::Endianess::Big){
                dirStream.writeBytes(reinterpret_cast<uint8_t*>(temp), 4);
            } else {
                std::swap(temp[0], temp[3]);
                std::swap(temp[1], temp[2]);
                dirStream.writeBytes(reinterpret_cast<uint8_t*>(temp), 4);
            }
        }

        dirStream.writeUInt32(stringTable[folderName]); // ??????
        dirStream.writeUInt16(Hash(folderName));
        dirStream.writeUInt16(folder->GetFileCount() + 2);
        dirStream.writeUInt32(currentFileIndex);

        // Write File entries
        for(auto file : folder->GetFiles()){
            fileStream.writeUInt16(currentFileIndex);
            fileStream.writeUInt16(Hash(file->GetName()));
            fileStream.writeUInt8(0x01 | 0x10);
            fileStream.writeUInt8(0x00);

            std::string fileName = file->GetName();
            fileStream.writeUInt16(stringTable[fileName]);
            fileStream.writeUInt32(fileDataStream.tell());
            fileStream.writeUInt32(file->GetSize());
            fileStream.writeUInt32(0x00);

            fileDataStream.writeBytes(file->GetData(), file->GetSize());

            uint32_t delta = Util::PadTo32(file->GetSize()) - file->GetSize();
            for(int x = 0; x < delta; x++){
                fileDataStream.writeUInt8(0);
            }

            currentFileIndex++;
        }

        // Write Subdirectory entries
        for(auto subdir : folder->GetSubdirectories()){
            auto subdirIter = std::find(mDirectories.begin(), mDirectories.end(), subdir);
            fileStream.writeUInt16(0xFFFF);
            fileStream.writeUInt16(Hash(subdir->GetName()));
            fileStream.writeUInt8(0x02);
            fileStream.writeUInt8(0x00);

            std::string subdirName = subdir->GetName();

            fileStream.writeUInt16(stringTable[subdirName]); // ????
            fileStream.writeUInt32(subdirIter - mDirectories.begin());
            fileStream.writeUInt32(0x10);
            fileStream.writeUInt32(0x00);
            currentFileIndex++;
        }


        // Write .
        // [v]: perhaps this should be put into a function of some kind? unsure...
        fileStream.writeUInt16(0xFFFF);
        fileStream.writeUInt16(Hash("."));
        fileStream.writeUInt8(0x02);
        fileStream.writeUInt8(0x00);
        fileStream.writeUInt16(0x00);
        fileStream.writeUInt32(i);
        fileStream.writeUInt32(0x10);
        fileStream.writeUInt32(0x00);

        // And write ..
        fileStream.writeUInt16(0xFFFF);
        fileStream.writeUInt16(Hash(".."));
        fileStream.writeUInt8(0x02);
        fileStream.writeUInt8(0x00);
        fileStream.writeUInt16(0x02);
        if(folder->GetParent().lock() != nullptr){
            auto dirIter = std::find(mDirectories.begin(), mDirectories.end(), folder->GetParent().lock());
            fileStream.writeUInt32(dirIter - mDirectories.begin());
        } else {
            fileStream.writeUInt32((uint32_t)-1);
        }
        fileStream.writeUInt32(0x10);
        fileStream.writeUInt32(0x00);

        currentFileIndex += 2; // account for . and .. file entries
    }

    // Write Header
    headerStream.writeUInt32(0x52415243);
    headerStream.writeUInt32(archiveSizes["total"]);
    headerStream.writeUInt32(fileSystemChunk - archiveData);
    headerStream.writeUInt32(fileDataChunk - fileSystemChunk);
    headerStream.writeUInt32(fileDataStream.getSize());
    headerStream.writeUInt32(fileDataStream.getSize()); //TODO: mram flag!
    headerStream.writeUInt32(0); //TODO: aram flag!
    headerStream.writeUInt32(0); // pad

    //Write FS Header
    fileSystemStream.writeUInt32(mDirectories.size());
    fileSystemStream.writeUInt32(dirChunk - fileSystemChunk);
    fileSystemStream.writeUInt32(currentFileIndex);
    fileSystemStream.writeUInt32(fileChunk - fileSystemChunk);
    fileSystemStream.writeUInt32(stringTableStream.getSize());
    fileSystemStream.writeUInt32(strTableChunk - fileSystemChunk);
    fileSystemStream.writeUInt16(currentFileIndex);
    fileSystemStream.writeUInt8(0);
    fileSystemStream.writeUInt8(0);
    fileSystemStream.writeUInt32(0);

    switch(compression){
        case Compression::Format::None:
            {
                bStream::CFileStream outFile(path.string(), mArchiveOrder, bStream::OpenMode::Out);
                outFile.writeBytes(archiveData, archiveSizes["total"]);
                if(padCompressed) while(outFile.tell() < Util::AlignTo(outFile.getSize(), 0x20)) { outFile.writeUInt8(0); }
            }
            break;

        // Compresison container, as far as I can tell, is always BE
        case Compression::Format::YAY0:
            {
                bStream::CMemoryStream archiveOut(archiveData, archiveSizes["total"], bStream::Endianess::Big, bStream::OpenMode::In);
                bStream::CFileStream outFile(path.string(), bStream::Endianess::Big, bStream::OpenMode::Out);

                Compression::Yay0::Compress(&archiveOut, &outFile);
                if(padCompressed) while(outFile.tell() < Util::AlignTo(outFile.getSize(), 0x20)) { outFile.writeUInt8(0); }
            }
            break;
        case Compression::Format::YAZ0:
            {
                bStream::CMemoryStream archiveOut(archiveData, archiveSizes["total"], bStream::Endianess::Big, bStream::OpenMode::In);
                bStream::CFileStream outFile(path.string(), bStream::Endianess::Big, bStream::OpenMode::Out);

                Compression::Yaz0::Compress(&archiveOut, &outFile, compressionLevel);
                if(padCompressed) while(outFile.tell() < Util::AlignTo(outFile.getSize(), 0x20)) { outFile.writeUInt8(0); }
            }
            break;
    }


    delete[] archiveData;
}

void Rarc::Save(std::vector<uint8_t>& buffer, Compression::Format compression, uint8_t compressionLevel, bool padCompressed){
    std::map<std::string, uint32_t> archiveSizes = CalculateArchiveSizes();

    uint8_t* archiveData = new uint8_t[archiveSizes["total"]];
    memset(archiveData, 0, archiveSizes["total"]);

    uint8_t* fileSystemChunk = archiveData + 0x20;
    uint8_t* dirChunk = archiveData + 0x40;
    uint8_t* fileChunk = archiveData + 0x40 + archiveSizes["dirEntries"];
    uint8_t* strTableChunk = archiveData + 0x40 + archiveSizes["dirEntries"] + archiveSizes["fileEntries"];
    uint8_t* fileDataChunk = archiveData + 0x40 + archiveSizes["dirEntries"] + archiveSizes["fileEntries"] + archiveSizes["strTable"];

    bStream::CMemoryStream headerStream(archiveData, 0x20, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream fileSystemStream(fileSystemChunk, 0x20, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream dirStream(dirChunk, archiveSizes["dirEntries"], mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream fileStream(fileChunk, archiveSizes["fileEntries"], mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream stringTableStream(strTableChunk, archiveSizes["strTable"], mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream fileDataStream(fileDataChunk, archiveSizes["fileData"], mArchiveOrder, bStream::OpenMode::Out);

    // Generate String Table

    std::map<std::string, uint32_t> stringTable{{".", 0x00}, {"..", 0x02}};

    stringTableStream.writeString(".");
    stringTableStream.writeUInt8(0x00);
    stringTableStream.writeString("..");
    stringTableStream.writeUInt8(0x00);

    for(auto dir : mDirectories){
        if(stringTable.count(dir->GetName()) == 0){
            stringTable.insert({dir->GetName(), stringTableStream.tell()});
            stringTableStream.writeString(dir->GetName());
            stringTableStream.writeUInt8(0x00);
        }
        for(auto file : dir->GetFiles()){
            if(stringTable.count(file->GetName()) == 0){
                stringTable.insert({file->GetName(), stringTableStream.tell()});
                stringTableStream.writeString(file->GetName());
                stringTableStream.writeUInt8(0x00);
            }
        }
    }

    std::size_t currentFileIndex = 0;

    // Write Archive Structure
    for(std::size_t i = 0; i < mDirectories.size(); i++)
    {
        std::shared_ptr<Folder> folder = mDirectories[i];

        std::string folderName = folder->GetName();

        //Write IDs
        if(i == 0){
            dirStream.writeUInt32(0x524F4F54);
        } else {
            char temp[4];
            memset(temp, 0x20, sizeof(temp));
            for (std::size_t ch = 0; ch < 4; ch++){
                if(ch >= folderName.size()) break;
                temp[ch] = toupper(folderName[ch]);
            }
            // This is a hack, these are technically ints but if we treat them that way the expected IDs end up broken in BE exports.
            // There is probably a better way to do this but im too lazy to fix it right now
            if(dirStream.getOrder() == bStream::Endianess::Big){
                dirStream.writeBytes(reinterpret_cast<uint8_t*>(temp), 4);
            } else {
                std::swap(temp[0], temp[3]);
                std::swap(temp[1], temp[2]);
                dirStream.writeBytes(reinterpret_cast<uint8_t*>(temp), 4);
            }
        }

        dirStream.writeUInt32(stringTable[folderName]); // ??????
        dirStream.writeUInt16(Hash(folderName));
        dirStream.writeUInt16(folder->GetFileCount() + 2);
        dirStream.writeUInt32(currentFileIndex);

        // Write File entries
        for(auto file : folder->GetFiles()){
            fileStream.writeUInt16(currentFileIndex);
            fileStream.writeUInt16(Hash(file->GetName()));
            fileStream.writeUInt8(0x01 | 0x10);
            fileStream.writeUInt8(0x00);

            std::string fileName = file->GetName();
            fileStream.writeUInt16(stringTable[fileName]);
            fileStream.writeUInt32(fileDataStream.tell());
            fileStream.writeUInt32(file->GetSize());
            fileStream.writeUInt32(0x00);

            fileDataStream.writeBytes(file->GetData(), file->GetSize());

            uint32_t delta = Util::PadTo32(file->GetSize()) - file->GetSize();
            for(int x = 0; x < delta; x++){
                fileDataStream.writeUInt8(0);
            }

            currentFileIndex++;
        }

        // Write Subdirectory entries
        for(auto subdir : folder->GetSubdirectories()){
            auto subdirIter = std::find(mDirectories.begin(), mDirectories.end(), subdir);
            fileStream.writeUInt16(0xFFFF);
            fileStream.writeUInt16(Hash(subdir->GetName()));
            fileStream.writeUInt8(0x02);
            fileStream.writeUInt8(0x00);

            std::string subdirName = subdir->GetName();

            fileStream.writeUInt16(stringTable[subdirName]); // ????
            fileStream.writeUInt32(subdirIter - mDirectories.begin());
            fileStream.writeUInt32(0x10);
            fileStream.writeUInt32(0x00);
            currentFileIndex++;
        }


        // Write .
        // [v]: perhaps this should be put into a function of some kind? unsure...
        fileStream.writeUInt16(0xFFFF);
        fileStream.writeUInt16(Hash("."));
        fileStream.writeUInt8(0x02);
        fileStream.writeUInt8(0x00);
        fileStream.writeUInt16(0x00);
        fileStream.writeUInt32(i);
        fileStream.writeUInt32(0x10);
        fileStream.writeUInt32(0x00);

        // And write ..
        fileStream.writeUInt16(0xFFFF);
        fileStream.writeUInt16(Hash(".."));
        fileStream.writeUInt8(0x02);
        fileStream.writeUInt8(0x00);
        fileStream.writeUInt16(0x02);
        if(folder->GetParent().lock() != nullptr){
            auto dirIter = std::find(mDirectories.begin(), mDirectories.end(), folder->GetParent().lock());
            fileStream.writeUInt32(dirIter - mDirectories.begin());
        } else {
            fileStream.writeUInt32((uint32_t)-1);
        }
        fileStream.writeUInt32(0x10);
        fileStream.writeUInt32(0x00);

        currentFileIndex += 2; // account for . and .. file entries
    }

    // Write Header
    headerStream.writeUInt32(0x52415243);
    headerStream.writeUInt32(archiveSizes["total"]);
    headerStream.writeUInt32(fileSystemChunk - archiveData);
    headerStream.writeUInt32(fileDataChunk - fileSystemChunk);
    headerStream.writeUInt32(fileDataStream.getSize());
    headerStream.writeUInt32(fileDataStream.getSize()); //TODO: mram flag!
    headerStream.writeUInt32(0); //TODO: aram flag!
    headerStream.writeUInt32(0); // pad

    //Write FS Header
    fileSystemStream.writeUInt32(mDirectories.size());
    fileSystemStream.writeUInt32(dirChunk - fileSystemChunk);
    fileSystemStream.writeUInt32(currentFileIndex);
    fileSystemStream.writeUInt32(fileChunk - fileSystemChunk);
    fileSystemStream.writeUInt32(stringTableStream.getSize());
    fileSystemStream.writeUInt32(strTableChunk - fileSystemChunk);
    fileSystemStream.writeUInt16(currentFileIndex);
    fileSystemStream.writeUInt8(0);
    fileSystemStream.writeUInt8(0);
    fileSystemStream.writeUInt32(0);


    switch(compression){
        case Compression::Format::None:
            {
                buffer.resize(archiveSizes["total"]);
                std::memcpy(buffer.data(), archiveData, archiveSizes["total"]);
            }
            break;

        case Compression::Format::YAY0:
            {
                bStream::CMemoryStream archiveOut(archiveData, archiveSizes["total"], bStream::Endianess::Big, bStream::OpenMode::In);
                bStream::CMemoryStream compressedOut(static_cast<std::size_t>(archiveSizes["total"]), bStream::Endianess::Big, bStream::OpenMode::Out);

                Compression::Yay0::Compress(&archiveOut, &compressedOut);

                buffer.resize(padCompressed ? Util::AlignTo(compressedOut.getSize(), 0x20) : compressedOut.getSize());
                std::memcpy(buffer.data(), compressedOut.getBuffer(), compressedOut.getSize());
            }
            break;
        case Compression::Format::YAZ0:
            {
                bStream::CMemoryStream archiveOut(archiveData, archiveSizes["total"], bStream::Endianess::Big, bStream::OpenMode::In);
                bStream::CMemoryStream compressedOut(static_cast<std::size_t>(archiveSizes["total"]), bStream::Endianess::Big, bStream::OpenMode::Out);

                Compression::Yaz0::Compress(&archiveOut, &compressedOut, compressionLevel);

                buffer.resize(padCompressed ? Util::AlignTo(compressedOut.getSize(), 0x20) : compressedOut.getSize());
                std::memcpy(buffer.data(), compressedOut.getBuffer(), compressedOut.getSize());
            }
            break;
    }


    delete[] archiveData;
}

// Needs error checking
bool Rarc::Load(bStream::CStream* stream){
    bStream::CMemoryStream decompressedArcStream(1, bStream::Endianess::Big, bStream::OpenMode::Out);

    bStream::CStream* rarcStream = stream;

    if(stream->getOrder() != bStream::Endianess::Big) stream->setOrder(bStream::Endianess::Big);

    uint32_t magic = stream->readUInt32();

    if(magic == 0x59617A30){
        decompressedArcStream.Reserve(Compression::GetDecompressedSize(stream));
        Compression::Yaz0::Decompress(stream, &decompressedArcStream);
        decompressedArcStream.changeMode(bStream::OpenMode::In);
        rarcStream = &decompressedArcStream;
    } else if(magic == 0x59617930){
        decompressedArcStream.Reserve(Compression::GetDecompressedSize(stream));
        Compression::Yay0::Decompress(stream, &decompressedArcStream);
        decompressedArcStream.changeMode(bStream::OpenMode::In);
        rarcStream = &decompressedArcStream;
    }

    rarcStream->seek(0);

    magic = rarcStream->readUInt32();

    if(magic == 0x43524152){
        rarcStream->setOrder(bStream::Endianess::Little);
        mArchiveOrder = bStream::Endianess::Little;
    }

    if(magic != 0x52415243 && magic != 0x43524152){
        return false;
    }

    rarcStream->seek(8); // Skip unneeded stuff

    uint32_t fsOffset = rarcStream->readUInt32();
    uint32_t fsSize = rarcStream->readUInt32();

    rarcStream->seek(fsOffset);

    uint32_t dirCount = rarcStream->readUInt32();
    uint32_t dirOffset = rarcStream->readUInt32();

    uint32_t fileCount = rarcStream->readUInt32();
    uint32_t fileOffset = rarcStream->readUInt32();

    uint32_t strTableSize = rarcStream->readUInt32();
    uint32_t strTableOffset = rarcStream->readUInt32();

    for(std::size_t i = 0; i < dirCount; i++) mDirectories.push_back(Folder::Create(GetPtr()));

    rarcStream->seek(dirOffset + fsOffset);
    for(std::size_t i = 0; i < dirCount; i++)
    {
        std::shared_ptr<Folder> folder = mDirectories[i];

        rarcStream->skip(4);
        uint32_t nameOffset = rarcStream->readUInt32();
        rarcStream->skip(2);

        uint16_t filenum = rarcStream->readUInt16();
        uint32_t fileoff = rarcStream->readUInt32();

        std::size_t readReturn = rarcStream->tell();

        rarcStream->seek(strTableOffset + fsOffset + nameOffset);
        char c;
        std::string dirName;
		while((c = rarcStream->readUInt8()) != '\0' && rarcStream->tell()  < (strTableOffset + fsOffset + strTableSize)){
			dirName.push_back(c);
		}

        folder->SetName(dirName);

        rarcStream->seek(readReturn);

        std::size_t pos = rarcStream->tell();
        rarcStream->seek(fileOffset + (fileoff * 0x14) + fsOffset);

        for(std::size_t f = 0; f < filenum; f++)
        {
            std::shared_ptr<File> file = File::Create();

            uint16_t id = rarcStream->readUInt16();
	        rarcStream->skip(2);

            uint32_t fileAttrs = rarcStream->readUInt32();
            uint32_t fileNameOffset = fileAttrs & 0x00FFFFFF;
            uint8_t attr = (fileAttrs >> 24) & 0xFF;
            uint32_t start = rarcStream->readUInt32();

            uint32_t fileSize = rarcStream->readUInt32();

            std::size_t readReturn = rarcStream->tell();

            rarcStream->seek(strTableOffset + fsOffset + fileNameOffset);

            char c;
            std::string name;
            while((c = rarcStream->readUInt8()) != '\0' && rarcStream->tell()  < (strTableOffset + fsOffset + strTableSize)){
                name.push_back(c);
            }

            rarcStream->seek(readReturn);

            rarcStream->skip(4);

            if(attr & 0x01){

                unsigned char* fileData = new unsigned char[fileSize];

                std::size_t pos = rarcStream->tell();
                rarcStream->seek(fsOffset + fsSize + start);
                rarcStream->readBytesTo(fileData, fileSize);
                rarcStream->seek(pos);

                file->SetName(name);
                file->SetData(fileData, fileSize);
                folder->AddFile(file);

                delete[] fileData;
            } else if(attr & 0x02) {
                if(start != -1 && name != ".." && name != "."){
                    folder->AddSubdirectory(mDirectories[start]);
                }
            }

        }
        rarcStream->seek(pos);
    }
    return true;
}
void U8::Save(std::vector<uint8_t>& buffer,
    Compression::Format compression,
    uint8_t compressionLevel,
    bool padCompressed)
{
    if (mDirectories.empty() || !mDirectories[0]) {
        buffer.clear();
        return;
    }

    struct NodeEntry {
        std::shared_ptr<Folder> folder;
        std::shared_ptr<File>   file;
    };

    std::vector<NodeEntry> nodes;
    nodes.reserve(4096);

    std::unordered_map<Folder*, std::pair<uint32_t, uint32_t>> dirRange;

    uint32_t curIndex = 0;
    std::unordered_set<Folder*> visited;
    std::function<void(std::shared_ptr<Folder>)> dfs =
        [&](std::shared_ptr<Folder> f)
    {
        if (!f)
            return;

        if (visited.count(f.get()))
            return;
        visited.insert(f.get());

        uint32_t myIndex = curIndex++;
        nodes.push_back({ f, nullptr });

        for (auto& sub : f->GetSubdirectories())
            dfs(sub);

        for (auto& file : f->GetFiles()) {
            nodes.push_back({ nullptr, file });
            ++curIndex;
        }


        dirRange[f.get()] = { myIndex + 1, curIndex };
    };

    dfs(mDirectories[0]);

    uint32_t nodeCount = nodes.size();

    std::map<std::string, uint32_t> stringTable;
    std::vector<uint8_t> stringBuf;
    stringBuf.push_back(0);

    auto addName = [&](const std::string& name) {
        if (name.empty()) return;
        if (stringTable.count(name)) return;

        uint32_t off = stringBuf.size();
        stringTable[name] = off;

        for (char c : name) stringBuf.push_back(c);
        stringBuf.push_back(0);
    };

    for (auto& n : nodes) {
        if (n.folder) addName(n.folder->GetName());
        if (n.file)   addName(n.file->GetName());
    }

    uint32_t stringSize = stringBuf.size();

    uint32_t nodeChunkOffset = 0x20;
    uint32_t nodeChunkSize = nodeCount * 0x0C;

    uint32_t stringTableOffset = nodeChunkOffset + nodeChunkSize;

    uint32_t headerSize = stringTableOffset + stringSize;

    uint32_t dataOffset = Util::AlignTo(headerSize, 0x20);

    uint32_t fileDataSize = 0;
    for (auto& n : nodes)
        if (n.file)
            fileDataSize += Util::PadTo32(n.file->GetSize());

    uint32_t totalSize = dataOffset + fileDataSize;

    std::vector<uint8_t> out(totalSize, 0);

    uint8_t* nodeChunk = out.data() + nodeChunkOffset;
    uint8_t* strChunk = out.data() + stringTableOffset;
    uint8_t* dataChunk = out.data() + dataOffset;

    bStream::CMemoryStream header(out.data(), 0x20, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream node(nodeChunk, nodeCount * 0x0C, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream str(strChunk, stringSize, mArchiveOrder, bStream::OpenMode::Out);
    bStream::CMemoryStream dat(dataChunk, fileDataSize, mArchiveOrder, bStream::OpenMode::Out);

    str.writeBytes(stringBuf.data(), stringBuf.size());

    for (uint32_t i = 0; i < nodeCount; i++) {
        auto& n = nodes[i];

        if (n.folder) {
            uint32_t nameOff = stringTable[n.folder->GetName()];
            uint32_t typeAndName = (1u << 24) | (nameOff & 0xFFFFFF);

            if (i == 0) {
                node.writeUInt32(0x01000000);
                node.writeUInt32(1);
                node.writeUInt32(nodeCount);
            }
            else {
                node.writeUInt32(typeAndName);
                auto [first, last] = dirRange[n.folder.get()];
                node.writeUInt32(first);
                node.writeUInt32(last);
            }
        }
        else {
            uint32_t nameOff = stringTable[n.file->GetName()];
            uint32_t typeAndName = nameOff & 0xFFFFFF;

            node.writeUInt32(typeAndName);

            uint32_t offset = dataOffset + dat.tell();
            node.writeUInt32(offset);
            node.writeUInt32(n.file->GetSize());

            dat.writeBytes(n.file->GetData(), n.file->GetSize());

            uint32_t pad = Util::PadTo32(n.file->GetSize()) - n.file->GetSize();
            for (uint32_t p = 0; p < pad; p++) dat.writeUInt8(0);
        }
    }
    header.writeUInt32(0x55AA382D);
    header.writeUInt32(0x20);
    header.writeUInt32(headerSize);
    header.writeUInt32(dataOffset);
    header.writeUInt32(0);
    header.writeUInt32(0);
    header.writeUInt32(0);
    header.writeUInt32(0);

    if (compression == Compression::Format::None) {
        buffer = out;
    }
    else {
        bStream::CMemoryStream in(out.data(), totalSize, bStream::Endianess::Big, bStream::OpenMode::In);
        bStream::CMemoryStream comp(totalSize, bStream::Endianess::Big, bStream::OpenMode::Out);

        if (compression == Compression::Format::YAY0)
            Compression::Yay0::Compress(&in, &comp);
        else
            Compression::Yaz0::Compress(&in, &comp, compressionLevel);

        uint32_t written = comp.tell();

        uint32_t finalSize = padCompressed ? Util::AlignTo(written, 0x20) : written;
        buffer.resize(finalSize);

        memcpy(buffer.data(), comp.getBuffer(), written);
    }
}
bool U8::Load(bStream::CStream* stream)
{
    bStream::CMemoryStream decompressed(1, bStream::Endianess::Big, bStream::OpenMode::Out);
    bStream::CStream* u8Stream = nullptr;

    if (stream->getOrder() != bStream::Endianess::Big)
        stream->setOrder(bStream::Endianess::Big);

    uint32_t magic = stream->readUInt32();

    if (magic == 0x59617A30) { // Yaz0
        decompressed.Reserve(Compression::GetDecompressedSize(stream));
        Compression::Yaz0::Decompress(stream, &decompressed);
        decompressed.changeMode(bStream::OpenMode::In);
        u8Stream = &decompressed;
        mDecompressed = decompressed;
    }
    else if (magic == 0x59617930) { // Yay0
        decompressed.Reserve(Compression::GetDecompressedSize(stream));
        Compression::Yay0::Decompress(stream, &decompressed);
        decompressed.changeMode(bStream::OpenMode::In);
        u8Stream = &decompressed;
        mDecompressed = decompressed;
    }
    else {
        stream->seek(0);
        size_t size = stream->getSize();

        mDecompressed = bStream::CMemoryStream((uint32_t)size, bStream::Endianess::Big, bStream::OpenMode::Out);
        std::vector<uint8_t> tmp(size);
        stream->readBytesTo(tmp.data(), size);
        mDecompressed.writeBytes(tmp.data(), size);
        mDecompressed.changeMode(bStream::OpenMode::In);

        u8Stream = &mDecompressed;
    }

    u8Stream->seek(0);
    magic = u8Stream->readUInt32();
    if (magic != 0x55AA382D)
        return false;

    uint32_t rootNodeOffset = u8Stream->readUInt32();
    uint32_t headerSize = u8Stream->readUInt32();
    uint32_t dataOffset = u8Stream->readUInt32();

    u8Stream->seek(rootNodeOffset);

    uint32_t typeAndName = u8Stream->readUInt32();
    uint32_t firstChild = u8Stream->readUInt32();
    uint32_t lastChild = u8Stream->readUInt32();

    uint32_t nodeCount = lastChild;

    if (rootNodeOffset + nodeCount * 0x0C > dataOffset) {
        nodeCount = lastChild + 1;
    }

    struct U8Node {
        uint32_t typeAndName;
        uint32_t dataOffsetOrFirst;
        uint32_t sizeOrLast;
    };

    std::vector<U8Node> nodes(nodeCount);

    nodes[0] = { typeAndName, firstChild, lastChild };

    for (uint32_t i = 1; i < nodeCount; i++) {
        nodes[i].typeAndName = u8Stream->readUInt32();
        nodes[i].dataOffsetOrFirst = u8Stream->readUInt32();
        nodes[i].sizeOrLast = u8Stream->readUInt32();
    }

    uint32_t stringTableOffset = rootNodeOffset + nodeCount * 0x0C;
    u8Stream->seek(stringTableOffset);

    std::vector<uint8_t> stringTable(dataOffset - stringTableOffset);
    u8Stream->readBytesTo(stringTable.data(), stringTable.size());

    auto getName = [&](uint32_t offset) -> std::string {
        if (offset == 0 || offset >= stringTable.size()) return "";
        std::string s;
        for (uint32_t i = offset; i < stringTable.size() && stringTable[i] != 0; i++)
            s.push_back(stringTable[i]);
        return s;
    };

    mDirectories.assign(nodeCount, nullptr);
    mFiles.assign(nodeCount, nullptr);

    unsigned char* base = reinterpret_cast<unsigned char*>(mDecompressed.getBuffer());

    for (uint32_t i = 0; i < nodeCount; i++) {
        uint32_t type = nodes[i].typeAndName >> 24;
        uint32_t nameOffset = nodes[i].typeAndName & 0xFFFFFF;

        if (type == 1) {
            auto folder = Folder::CreateU8(GetPtr());
            folder->SetName(getName(nameOffset));
            mDirectories[i] = folder;
        }
        else {
            auto file = File::Create();
            file->SetName(getName(nameOffset));

            uint32_t rel = nodes[i].dataOffsetOrFirst;
            uint32_t size = nodes[i].sizeOrLast;

            file->SetView(base + rel, size);
            mFiles[i] = file;
        }
    }

    if (nodeCount == 0 || !mDirectories[0])
        return true;

    struct StackEntry {
        std::shared_ptr<Folder> folder;
        uint32_t endIndex;
    };

    std::vector<StackEntry> stack;
    stack.push_back({ mDirectories[0], nodes[0].sizeOrLast });

    for (uint32_t i = 1; i < nodeCount; ++i) {

        while (!stack.empty() && i >= stack.back().endIndex)
            stack.pop_back();

        auto parent = stack.empty() ? nullptr : stack.back().folder;

        uint32_t type = nodes[i].typeAndName >> 24;

        if (type == 1) {
            auto folder = mDirectories[i];
            if (parent)
                parent->AddSubdirectoryU8(folder);

            uint32_t last = nodes[i].sizeOrLast;
            stack.push_back({ folder, last });
        }
        else {
            auto file = mFiles[i];
            if (parent)
                parent->AddFile(file);
        }
    }
    return true;
}
    void U8::SaveToFile(std::filesystem::path path,
        Compression::Format compression,
        uint8_t compressionLevel,
        bool padCompressed)
    {
        std::vector<uint8_t> buffer;
        Save(buffer, compression, compressionLevel, padCompressed);

        bStream::CFileStream outFile(path.string(),
            bStream::Endianess::Big,
            bStream::OpenMode::Out);

        outFile.writeBytes(buffer.data(), buffer.size());
    }
}
