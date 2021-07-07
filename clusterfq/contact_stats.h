#pragma once

#include <time.h>

using namespace std;

enum contact_stats_event {
	CSE_CHUNK_SENT,
	CSE_CHUNK_RECEIVED,
	CSE_RECEIPT_RECEIVED,
	CSE_RECEIPT_SENT,
	CSE_TIME_PER_256_CHUNK_IN,
	CSE_TIME_PER_256_CHUNK_OUT,
	CSE_POLLUTION,
	CSE_MALFORMED
};

struct contact_stats_element {
	unsigned long long		chunks_sent;
	unsigned long long		chunks_received;
	unsigned long long		receipts_sent;
	unsigned long long		receipts_received;

	float					moving_average_time_per_256_chunk_in;
	unsigned int			moving_average_time_per_256_chunk_in_data_count;

	float					moving_average_time_per_256_chunk_out;
	unsigned int			moving_average_time_per_256_chunk_out_data_count;

	unsigned long long		pollution_count;
	unsigned long long		malformed_count;
};

struct contact_stats {
	time_t last_seen;

	struct contact_stats_element hours_of_days_of_week[24 * 7];
};

void contact_stats_init(struct contact_stats* cs);

void contact_stats_update(struct contact_stats* cs, enum contact_stats_event cse, float data = 0.0f);

void contact_stats_dump(struct contact_stats* cs, unsigned int day, unsigned int hour);