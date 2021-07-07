#include "contact_stats.h"

#include <cstring>
#include <algorithm>
#include <iostream>

void contact_stats_init(struct contact_stats* cs) {
	cs->last_seen = 0;
	memset(&cs->hours_of_days_of_week, 0, 7 * 24 * sizeof(struct contact_stats_element));
}

void contact_stats_update(struct contact_stats* cs, enum contact_stats_event cse, float data) {
	time_t now = time(nullptr);
	tm* gmtm = gmtime(&now);

	struct contact_stats_element* cs_elem = &cs->hours_of_days_of_week[gmtm->tm_wday * 24 + gmtm->tm_hour];

	if (cse == CSE_CHUNK_SENT) {
		cs_elem->chunks_sent++;
	} else if (cse == CSE_CHUNK_RECEIVED) {
		cs_elem->chunks_received++;
		cs->last_seen = now;
	} else if (cse == CSE_RECEIPT_SENT) {
		cs_elem->receipts_sent++;
	} else if (cse == CSE_RECEIPT_RECEIVED) {
		cs_elem->receipts_received++;
		cs->last_seen = now;
	} else if (cse == CSE_TIME_PER_256_CHUNK_IN) {
		cs_elem->moving_average_time_per_256_chunk_in_data_count++;
		int max_d = max(cs_elem->moving_average_time_per_256_chunk_in_data_count, 100u);
		cs_elem->moving_average_time_per_256_chunk_in = (   (max_d - 1)	/(float)max_d) * cs_elem->moving_average_time_per_256_chunk_in
														+ (	     1.0f	/(float)max_d) * data;
	} else if (cse == CSE_TIME_PER_256_CHUNK_OUT) {
		cs_elem->moving_average_time_per_256_chunk_out_data_count++;
		int max_d = max(cs_elem->moving_average_time_per_256_chunk_out_data_count, 100u);
		cs_elem->moving_average_time_per_256_chunk_out = (   (max_d - 1)/(float)max_d) * cs_elem->moving_average_time_per_256_chunk_out
														+ (	     1.0f	/(float)max_d) * data;
	} else if (cse == CSE_POLLUTION) {
		cs_elem->pollution_count++;
	} else if (cse == CSE_MALFORMED) {
		cs_elem->malformed_count++;
	}
}

void contact_stats_dump(struct contact_stats* cs, unsigned int day, unsigned int hour) {
	std::cout << std::endl;
	struct contact_stats_element* cs_elem = &cs->hours_of_days_of_week[day * 24 + hour];
	std::cout << "chunks_sent: " << cs_elem->chunks_sent << std::endl;
	std::cout << "chunks_received: " << cs_elem->chunks_received << std::endl;
	std::cout << "receipts_received: " << cs_elem->receipts_received << std::endl;
	std::cout << "receipts_sent: " << cs_elem->receipts_sent << std::endl;
	std::cout << "avg_time_per_256_in: " << cs_elem->moving_average_time_per_256_chunk_in << std::endl;
	std::cout << "avg_time_per_256_in_count: " << cs_elem->moving_average_time_per_256_chunk_in_data_count << std::endl;
	std::cout << "avg_time_per_256_out: " << cs_elem->moving_average_time_per_256_chunk_out << std::endl;
	std::cout << "avg_time_per_256_out_count: " << cs_elem->moving_average_time_per_256_chunk_out_data_count << std::endl;
	std::cout << "pollution_count: " << cs_elem->pollution_count << std::endl;
	std::cout << "malformed_count: " << cs_elem->malformed_count << std::endl;
	tm* gmtm = gmtime(&cs->last_seen);
	std::cout << "last_seen (utc): " << asctime(gmtm) << std::endl;
	std::cout << std::endl;
}