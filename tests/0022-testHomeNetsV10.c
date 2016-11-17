/*
  Copyright (C) 2016 Eneo Tecnologia S.L.
  Author: Eugenio Perez <eupm90@gmail.com>
  Based on Luca Deri nprobe 6.22 collector

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "f2k.h"

#include "rb_netflow_test.h"

#include <setjmp.h>
#include <cmocka.h>

/// @todo template+flow in the same message
struct TestV10Template{
	IPFIXFlowHeader flowHeader;
	IPFIXSet flowSetHeader;
	V9TemplateDef templateHeader; /* It's the same */
	const uint8_t templateBuffer[92];
};

/// @todo template+flow in the same message
struct TestV10Template_v6{
	IPFIXFlowHeader flowHeader;
	IPFIXSet flowSetHeader;
	V9TemplateDef templateHeader; /* It's the same */
	const uint8_t templateBuffer[92];
};

struct TestV10Flow{
	IPFIXFlowHeader flowHeader;
	IPFIXSet flowSetHeader;
	const uint8_t buffer1[0x61 - sizeof(IPFIXSet) - sizeof(IPFIXFlowHeader)];
}__attribute__((packed));

struct TestV10Flow_v6{
	IPFIXFlowHeader flowHeader;
	IPFIXSet flowSetHeader;
	const uint8_t buffer1[0xdf - sizeof(IPFIXSet) - sizeof(IPFIXFlowHeader)];
}__attribute__((packed));

static const struct TestV10Template v10Template = {
	.flowHeader = {
		/*uint16_t*/ .version = 0x0a00,           /* Current version=9*/
		/*uint16_t*/ .len = 0x7400,           /* The number of records in PDU. */
		/*uint32_t*/ .unix_secs = 0xdd5d6952,     /* Current time in msecs since router booted */
		/*uint32_t*/ .flow_sequence = 0x38040000, /* Sequence number of total flows seen */
		/*uint32_t*/ .observation_id = 0x00010000,      /* Source id */
	},

	.flowSetHeader = {
		/*uint16_t*/ .set_id = 0x0200,
		/*uint16_t*/ .set_len = 0x6400,
	},

	.templateHeader = {
		/*uint16_t*/ .templateId = 0x0d01, /*269*/
		/*uint16_t*/ .fieldCount = 0x1300,
	},

	.templateBuffer = {
		0x00, 0x08, 0x00, 0x04, /* SRC ADDR */
		0x00, 0x0c, 0x00, 0x04, /* DST ADDR */
		0x00, 0x3c, 0x00, 0x01, /* IP VERSION */
		0x00, 0x04, 0x00, 0x01, /* PROTO */
		0x00, 0x07, 0x00, 0x02, /* SRC PORT */
		0x00, 0x0b, 0x00, 0x02, /* DST PORT */
		0x00, 0x88, 0x00, 0x01, /* flowEndreason */
		0x00, 0xef, 0x00, 0x01, /* biflowDirection */
		0x00, 0x30, 0x00, 0x01, /* FLOW_SAMPLER_ID */
		0x01, 0x18, 0x00, 0x08, /* TRANSACTION_ID */
		0x00, 0x5f, 0x00, 0x04, /* APPLICATION ID*/
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0x00, 0x01, 0x00, 0x08, /* BYTES: */
		0x00, 0x02, 0x00, 0x04, /* PKTS*/
		0x00, 0x16, 0x00, 0x04, /* FIRST_SWITCHED */
		0x00, 0x15, 0x00, 0x04, /* LAST_SWITCHED*/
	}
};

static const struct TestV10Flow v10Flow = {
	.flowHeader = {
		/*uint16_t*/ .version = 0x0a00,           /* Current version=9*/
		/*uint16_t*/ .len = 0x6100,           /* The number of records in PDU. */
		/*uint32_t*/ .unix_secs = 0xdd5d6952,     /* Current time in msecs since router booted */
		/*uint32_t*/ .flow_sequence = 0x38040000, /* Sequence number of total flows seen */
		/*uint32_t*/ .observation_id = 0x00010000,      /* Source id */
	},

	.flowSetHeader = {
		/*uint16_t*/ .set_id = 0x0d01,
		/*uint16_t*/ .set_len = 0x5100,
	},

	.buffer1 = {
		0x0a, 0x0d, 0x7a, 0x2c, /* SRC ADDR 10.13.122.44 */
		0x42, 0xdc, 0x98, 0x13, /* DST ADDR 66.220.152.19*/
		0x04,                   /* IP VERSION: 4 */
		0x06,                   /* PROTO: 6 */
		0xd5, 0xb9,             /* SRC PORT: 54713 */
		0x01, 0xbb,             /* DST PORT: 443 */
		0x03,                   /* flowEndreason */
		0x01,                   /* biflowDirection */
		0x00,                   /* FLOW_SAMPLER_ID */
		0x8f, 0x63, 0xf3, 0x40, 0x00, 0x01, 0x00, 0x00, /* TRANSACTION_ID */
		0x0d, 0x00, 0x01, 0xc5, /* APPLICATION ID 13:453 */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x01, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x02, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x03, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x04, /* CISCO_URL */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb8, /* BYTES: 2744 */
		0x00, 0x00, 0x00, 0x1f, /* PKTS: 31*/
		0x0f, 0xed, 0x0a, 0xc0, /* FIRST_SWITCHED:  */
		0x0f, 0xee, 0x18, 0x00, /* LAST_SWITCHED: */
	},
};

static const struct TestV10Template_v6 v10Template_v6 = {
	.flowHeader = {
		/*uint16_t*/ .version = 0x0a00,           /* Current version=9*/
		/*uint16_t*/ .len = 0x7400,           /* The number of records in PDU. */
		/*uint32_t*/ .unix_secs = 0xdd5d6952,     /* Current time in msecs since router booted */
		/*uint32_t*/ .flow_sequence = 0x38040000, /* Sequence number of total flows seen */
		/*uint32_t*/ .observation_id = 0x00010000,      /* Source id */
	},

	.flowSetHeader = {
		/*uint16_t*/ .set_id = 0x0200,
		/*uint16_t*/ .set_len = 0x6400,
	},

	.templateHeader = {
		/*uint16_t*/ .templateId = 0x0d02, /*269*/
		/*uint16_t*/ .fieldCount = 0x1300,
	},

	.templateBuffer = {
		0x00, 0x1b, 0x00, 0x10, /* SRC ADDR */
		0x00, 0x1c, 0x00, 0x10, /* DST ADDR */
		0x00, 0x3c, 0x00, 0x01, /* IP VERSION */
		0x00, 0x04, 0x00, 0x01, /* PROTO */
		0x00, 0x07, 0x00, 0x02, /* SRC PORT */
		0x00, 0x0b, 0x00, 0x02, /* DST PORT */
		0x00, 0x88, 0x00, 0x01, /* flowEndreason */
		0x00, 0xef, 0x00, 0x01, /* biflowDirection */
		0x00, 0x30, 0x00, 0x01, /* FLOW_SAMPLER_ID */
		0x01, 0x18, 0x00, 0x08, /* TRANSACTION_ID */
		0x00, 0x5f, 0x00, 0x04, /* APPLICATION ID*/
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0xaf, 0xcb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x09, /* CISCO_URL, variable length */
		0x00, 0x01, 0x00, 0x08, /* BYTES: */
		0x00, 0x02, 0x00, 0x04, /* PKTS*/
		0x00, 0x16, 0x00, 0x04, /* FIRST_SWITCHED */
		0x00, 0x15, 0x00, 0x04, /* LAST_SWITCHED*/
	}
};

static const struct TestV10Flow_v6 v10Flow_v6 = {
	.flowHeader = {
		/*uint16_t*/ .version = 0x0a00,           /* Current version=9*/
		/*uint16_t*/ .len = 0xdf00,           /* The number of records in PDU. */
		/*uint32_t*/ .unix_secs = 0xdd5d6952,     /* Current time in msecs since router booted */
		/*uint32_t*/ .flow_sequence = 0x38040000, /* Sequence number of total flows seen */
		/*uint32_t*/ .observation_id = 0x00010000,      /* Source id */
	},

	.flowSetHeader = {
		/*uint16_t*/ .set_id = 0x0d02,
		/*uint16_t*/ .set_len = 0xcf00,
	},

	.buffer1 = {
		0x20, 0x01, 0x04, 0x28, 0xce, 0x00, 0x20, 0x11, 0x0d, 0x5a, 0x60, 0x69, 0x24, 0x67, 0x9b, 0xd1, /* SRC ADDR */
		0x20, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* DST ADDR */
		0x04,                   /* IP VERSION: 4 */
		0x06,                   /* PROTO: 6 */
		0xd5, 0xb9,             /* SRC PORT: 54713 */
		0x01, 0xbb,             /* DST PORT: 443 */
		0x03,                   /* flowEndreason */
		0x01,                   /* biflowDirection */
		0x00,                   /* FLOW_SAMPLER_ID */
		0x8f, 0x63, 0xf3, 0x40, 0x00, 0x01, 0x00, 0x00, /* TRANSACTION_ID */
		0x0d, 0x00, 0x01, 0xc5, /* APPLICATION ID 13:453 */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x01, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x02, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x03, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x04, /* CISCO_URL */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb8, /* BYTES: 2744 */
		0x00, 0x00, 0x00, 0x1f, /* PKTS: 31*/
		0x0f, 0xee, 0x18, 0x00, /* FIRST_SWITCHED:  */
		0x0f, 0xee, 0x18, 0x00, /* LAST_SWITCHED: */

		0x20, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* SRC ADDR */
		0x20, 0x01, 0x04, 0x28, 0xce, 0x00, 0x20, 0x11, 0x0d, 0x5a, 0x60, 0x69, 0x24, 0x67, 0x9b, 0xd1, /* DST ADDR */
		0x04,                   /* IP VERSION: 4 */
		0x06,                   /* PROTO: 6 */
		0xd5, 0xb9,             /* SRC PORT: 54713 */
		0x01, 0xbb,             /* DST PORT: 443 */
		0x03,                   /* flowEndreason */
		0x01,                   /* biflowDirection */
		0x00,                   /* FLOW_SAMPLER_ID */
		0x8f, 0x63, 0xf3, 0x40, 0x00, 0x01, 0x00, 0x00, /* TRANSACTION_ID */
		0x0d, 0x00, 0x01, 0xc5, /* APPLICATION ID 13:453 */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x01, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x02, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x03, /* CISCO_URL */
		0x06, 0x03, 0x00, 0x00, 0x50, 0x34, 0x04, /* CISCO_URL */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb8, /* BYTES: 2744 */
		0x00, 0x00, 0x00, 0x1f, /* PKTS: 31*/
		0x0f, 0xee, 0x18, 0x00, /* FIRST_SWITCHED:  */
		0x0f, 0xee, 0x18, 0x00, /* LAST_SWITCHED: */
	},
};

static const struct checkdata_value checkdata_values1[] = {
	{.key = "type", .value="netflowv10"},
	{.key = "src", .value="10.13.122.44"},
	{.key = "src_net", .value="10.13.30.0/16"},
	{.key = "src_net_name", .value="users"},
	{.key = "dst", .value="66.220.152.19"},
	{.key = "dst_net", .value=NULL},
	{.key = "dst_net_name", .value=NULL},
	{.key = "direction", .value="ingress"},
};

static const struct checkdata_value checkdata_values_v6_1[] = {
	{.key = "type", .value="netflowv10"},
	{.key = "src", .value="2001:0428:ce00:2011:0d5a:6069:2467:9bd1"},
	{.key = "src_net", .value="2001:0428:ce00:0000:0000:0000:0000:0000/48"},
	{.key = "src_net_name", .value="users6"},
	{.key = "dst", .value="2001:0008:0000:0000:0000:0000:0000:0001"},
	{.key = "dst_net", .value=NULL},
	{.key = "dst_net_name", .value=NULL},
	{.key = "direction", .value="ingress"},
};

static const struct checkdata_value checkdata_values_v6_2[] = {
	{.key = "type", .value="netflowv10"},
	{.key = "src", .value="2001:0008:0000:0000:0000:0000:0000:0001"},
	{.key = "src_net", .value=NULL},
	{.key = "src_net_name", .value=NULL},
	{.key = "dst", .value="2001:0428:ce00:2011:0d5a:6069:2467:9bd1"},
	{.key = "dst_net", .value="2001:0428:ce00:0000:0000:0000:0000:0000/48"},
	{.key = "dst_net_name", .value="users6"},
	{.key = "direction", .value="egress"},
};

static int prepare_test_nf10_home_nets(void **state) {
	static const struct checkdata checkdata_v4[] = {
		{.size = RD_ARRAYSIZE(checkdata_values1), .checks=checkdata_values1}
	};
	static const struct checkdata checkdata_v6[] = {
		{.size = RD_ARRAYSIZE(checkdata_values_v6_1), .checks=checkdata_values_v6_1},
		{.size = RD_ARRAYSIZE(checkdata_values_v6_2), .checks=checkdata_values_v6_2}
	};

#define TEST(config_path, mhosts_db_path, nf_dev_ip, mrecord, mrecord_size,    \
							checks, checks_sz) {   \
		.config_json_path = config_path,                               \
		.host_list_path = mhosts_db_path,                              \
		.netflow_src_ip = nf_dev_ip,                                   \
		.record = mrecord, .record_size = mrecord_size,                \
		.checkdata = checks, .checkdata_size = checks_sz               \
	}

#define TEST_TEMPLATE_FLOW0(config_path, mhosts_db_path, nf_dev_ip, template,  \
			template_size, flow, flow_size, checks, checks_sz)     \
	TEST(config_path, mhosts_db_path, nf_dev_ip, template, template_size,  \
								NULL, 0),      \
	TEST(NULL, NULL, nf_dev_ip, flow, flow_size, checks, checks_sz)

#define TEST_TEMPLATE_FLOW_V4(config_path, mhosts_db_path, nf_dev_ip)          \
		TEST_TEMPLATE_FLOW0(config_path, mhosts_db_path, nf_dev_ip,    \
			&v10Template, sizeof(v10Template),                     \
			&v10Flow, sizeof(v10Flow),                             \
			checkdata_v4, RD_ARRAYSIZE(checkdata_v4))

#define TEST_TEMPLATE_FLOW_V6(config_path, mhosts_db_path, nf_dev_ip)          \
		TEST_TEMPLATE_FLOW0(config_path, mhosts_db_path, nf_dev_ip,    \
			&v10Template_v6, sizeof(v10Template_v6),               \
			&v10Flow_v6, sizeof(v10Flow_v6),                       \
			checkdata_v6, RD_ARRAYSIZE(checkdata_v6))

	struct test_params test_params[] = {
		TEST_TEMPLATE_FLOW_V4("./tests/0022-testHomeNetsV10.json",
					"./tests/0011-data/", 0x04030201),

		TEST_TEMPLATE_FLOW_V4(NULL, NULL, 0x04030301),
		TEST_TEMPLATE_FLOW_V4(NULL, NULL, 0x04030401),
		TEST_TEMPLATE_FLOW_V6(NULL, NULL, 0x04030201),
		TEST_TEMPLATE_FLOW_V6(NULL, NULL, 0x04030301),
		TEST_TEMPLATE_FLOW_V6(NULL, NULL, 0x04030401),
	};

	*state = prepare_tests(test_params, RD_ARRAYSIZE(test_params));
	return *state == NULL;
}

int main() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(testFlow,
				prepare_test_nf10_home_nets, check_flow),
	};

	return cmocka_run_group_tests(tests, nf_test_setup, nf_test_teardown);
}
