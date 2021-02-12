/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.

    Copyright (c) 2011-2020 by Jiayin Cao - All rights reserved.

    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#include "display_mgr.h"
#include "core/socket_mgr.h"
#include "core/sassert.h"
#include "texture/rendertarget.h"

#ifdef SORT_IN_WINDOWS
#else
#define INVALID_SOCKET (-1)
#endif

enum Type : char {
    OpenImage = 0,
    ReloadImage = 1,
    CloseImage = 2,
    UpdateImage = 3,
    CreateImage = 4,
};

static std::string  channelNames[] = { "R", "G", "B" };
static int          nChannels = 3;

void DisplayManager::AddDisplayServer(const std::string host, const std::string& port) {
    // It is fairly easy to support multiple display servers, but it makes little sense for me for now.
    // So I will ignore the second one coming in.
    if (m_stream)
        return;

    m_socket = SocketManager::GetSingleton().AddSocket(SOCKET_TYPE::CLIENT, host, port);
    m_stream = std::make_unique<OSocketStream>(m_socket);
}

bool DisplayManager::IsDisplayServerConnected() const {
    return m_stream != nullptr && ( CONNECTION_FAILED != m_status );
}

void DisplayManager::ResolveDisplayServerConnection(){
    m_status = SocketManager::GetSingleton().ResolveSocket(m_socket) ? CONNECTION_SUCCEED : CONNECTION_FAILED;
}

void DisplayManager::ProcessDisplayQueue(int cnt) {
    // bail if connection is not established.
    if(m_status != CONNECTION_SUCCEED)
        return;

    // we only process 4 display tiles everytime this thread gains control
    while (cnt-- != 0) {
        std::shared_ptr<DisplayItemBase> item;
       
        // grab one from the queue
        {
            std::lock_guard<spinlock_mutex> guard(m_lock);

            // make sure there is still things left to do
            if (m_queue.empty())
                break;
            
            item = m_queue.front();
            m_queue.pop();
        }

        item->Process(m_stream);
    }
}

void DisplayManager::QueueDisplayItem(std::shared_ptr<DisplayItemBase> item) {
    std::lock_guard<spinlock_mutex> guard(m_lock);
    m_queue.push(item);
}

void DisplayTile::Process(std::unique_ptr<OSocketStream>& ptr_stream) {
    OSocketStream& stream = *ptr_stream;

    if (is_blender_mode){
        // [0] Length of the package, it doesn't count itself
        // [1] Width of the tile
        // [2] Height of the tile
        // [3] x position of the tile
        // [4] y position of the tile
        // [....] the pixel data
        const int pixel_memory_size = w * h * sizeof(float) * 4;
        const int total_size = pixel_memory_size + sizeof(int) * 4;
        stream << int(total_size);
        stream << w << h << x << y;
        stream.Write((char*)m_data->get(), pixel_memory_size);
        stream.Flush();
    } else {
        // This is for TEV
        // https://github.com/Tom94/tev
        for (auto i = 0u; i < nChannels; ++i) {
            stream << int(0);            // reserved for length
            stream << char(UpdateImage); // indicate to update some of the images
            stream << char(0);           // indicate to grab the current image
            stream << title;             // indicate the title of the image
            stream << channelNames[i];   // the channel name

            stream << x << y << w << h;
            stream.Write((char*)m_data[i].get(), w * h * sizeof(float));

            // set the length data
            const auto pos = stream.GetPos();
            stream.Seek(0);
            stream << int(pos);
            stream.Seek(pos);

            // distribute data to all display servers
            stream.Flush();
        }
    }
}

void DisplayImageInfo::Process(std::unique_ptr<OSocketStream>& ptr_stream) {
    // Blender doesn't care about creating a new image, only TEV does.
    if (!is_blender_mode) {
        OSocketStream& stream = *ptr_stream;

        const int image_width = w;
        const int image_height = h;
        stream << int(0);            // reserved for length
        stream << char(CreateImage); // indicate to update some of the images
        stream << char(1);           // indicate to grab the current image
        stream << title;             // indicate the title of the image
        stream << image_width << image_height;
        stream << int(nChannels);
        for (auto i = 0; i < nChannels; ++i)
            stream << channelNames[i];   // the channel name

        // set the length data
        const auto pos = stream.GetPos();
        stream.Seek(0);
        stream << int(pos);
        stream.Seek(pos);

        // flush the data
        stream.Flush();
    }
}

void TerminateIndicator::Process(std::unique_ptr<OSocketStream>& ptr_stream) {
    // Tev won't response this well
    if (is_blender_mode) {
        // Blender doesn't care about creating a new image, only TEV does.
        OSocketStream& stream = *ptr_stream;
        stream << int(0);   // 0 as length indicating that we are done, no more package will be received.
        stream.Flush();
    }
}

void FullTargetUpdate::Process(std::unique_ptr<OSocketStream>& ptr_stream) {
    // This is slow, but so far it is only used in light tracing algorithm, an algorithm that I rarely used.
    // So I don't care about its performance.
    // For bidirecitonal path tracing, by the time it is needed, it is already finished, so it is not a performance issue.
    // WARNING, this thread might result in some unknown results because of unguarded data racing. However, it is not a big
    // deal to reveal slightly inconsistent data as long as the final result is fine.

    const auto total_pixel = w * h;
    auto display_tile = std::make_shared<DisplayTile>();
    display_tile->x = 0;
    display_tile->y = 0;
    display_tile->w = w;
    display_tile->h = h;
    display_tile->title = title;
    display_tile->is_blender_mode = is_blender_mode;

    if (is_blender_mode) {
        display_tile->m_data[0] = std::make_unique<float[]>(total_pixel * 4);
    }
    else {
        for (auto i = 0u; i < 3; ++i)
            display_tile->m_data[i] = std::make_unique<float[]>(total_pixel);
    }

    for( auto i = 0u ; i < h;  ++i ){
        for( auto j = 0u ; j < w; ++j ){
            const auto& color = m_rt->GetColor(j, i);
            if (is_blender_mode) {
                auto local_index = j + (h - 1 - i) * w;
                display_tile->m_data[0][4 * local_index] = color[0];
                display_tile->m_data[0][4 * local_index + 1] = color[1];
                display_tile->m_data[0][4 * local_index + 2] = color[2];
                display_tile->m_data[0][4 * local_index + 3] = 1.0f;
            } else {
                for (auto c = 0u; c < RGBSPECTRUM_SAMPLE; ++c) {
                    const auto local_index = i * w + j;
                    display_tile->m_data[c][local_index] = color[c];
                }
            }
        }
    }

    display_tile->Process(ptr_stream);
}