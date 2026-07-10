import subprocess

# Test sequence design (derived earlier):
# Readings 1-8  : normal values, fills the buffer completely (buffer_size = 8)
# Reading 9     : deliberate spike
# Readings 10-17: normal values again -- by reading 17, the spike (written to
#                 slot 0 on reading 9) gets overwritten, fully cycling it out
#                 of the buffer.
#
# Expected result at cycle 17:
#   range_max  -> back down to normal range (spike no longer in buffer)
#   global_max -> still shows the spike (permanently recorded, never reset)

normal_values = [1800, 1850, 1820, 1790, 1830, 1810, 1840, 1795]  # readings 1-8
spike_value   = [3900]                                            # reading 9
normal_again  = [1805, 1815, 1825, 1790, 1835, 1800, 1820, 1810]  # readings 10-17

sequence = normal_values + spike_value + normal_again

stimulus = "\n".join(str(v) for v in sequence)

result = subprocess.run(
    ["./s01_sil"],
    input=stimulus,
    capture_output=True,
    text=True
)

print(result.stdout)

if result.returncode != 0:
    print("SIL run failed:", result.stderr)
