/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "core/Profile.h"


namespace vkp 
{
    std::vector<Profile::Record> Profile::GetRecordsFromLatest()
    {
        std::vector<Record> records;
        records.reserve(s_Records.size());

        InternalRecord* head = s_LatestRecord;
        while (head != nullptr)
        {
            records.emplace_back(head->name, head->duration, head->fileName);
            head = head->prev;
        }

        return records;
    }

    void Profile::InsertRecord(const Record& r)
    {
        InternalRecord record(r);

        if (s_LatestRecord != nullptr && *s_LatestRecord == record)
            s_LatestRecord->duration = record.duration;
        else
        {
            auto [pair, inserted] = s_Records.try_emplace(r.name, record);
            auto& it = pair->second;
            if (!inserted)
            {
                it.duration = record.duration;
                s_LatestRecord->next = &it;

                if (it.prev != nullptr)
                    it.prev->next = it.next;

                it.next->prev = it.prev;

                it.prev = s_LatestRecord;
                it.next = nullptr;
            }
            else
            {
                it.prev = s_LatestRecord;
                it.next = nullptr;
                if (s_LatestRecord != nullptr)
                    s_LatestRecord->next = &it;
            }

            s_LatestRecord = &it;
        }
    }

} // namespace vkp 
