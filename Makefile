run:
	env | grep VULK
	RUST_BACKTRACE=1 cargo run

.PHONY: run
