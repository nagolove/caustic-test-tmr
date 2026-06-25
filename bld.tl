return {
	{
		not_dependencies = {
			"lfs", "resvg", "rlwr",
		},
		artifact = "caustic-test-tmr",
		kind = 'app',
		main = "tmr-test.c",
		src = "src",
		common_define = {
			KOH_TMR_TEST = "1",
		},
	},
}
