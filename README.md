# CVE-2023-4911 - Looney Tunables

This is a (atm very rough) proof of concept for CVE-2023-4911. So far I've only verified it works on
Ubuntu 22.10 kinetic. Current version of the exploit contains a fair amount of "magic" offsets. If you have suggestions
on how to improve the heap shaping, feel free to send a PR my way :).  
  
This exploit is basically an implementation of [Qualys' excellent write up][qualys]. It deviates in
some places. That's not necessarily because I think I know better, this is just what worked on
my VM. There is probably room for improvement.

## Usage

1. Compile the exploit and suid library using `make`.
2. Run `./exploit` from a writable directory, containing both `suid_lib.so` and `exploit`. It'll create a folder called `U` in the current
   directory and populate it with the necessary libraries.

In my limited testing, the exploit needs somewhere between 4000-8000 attempts. The stack spray is not quite as good as Qualys' implementation, so it takes a bit longer.

Here's an example of the exploit in action (sped up): https://youtu.be/uw0EJ5zGEKE

## Improvement ideas
If you feel like hacking on this, here are some ideas to get you started:
- Make it self-contained. See https://github.com/ly4k/PwnKit for inspiration.
- Make the heap shaping more robust. Right now it's very fragile and depends on a lot of magic
  offsets.
- Make it work on more distros.

[qualys]: https://www.qualys.com/2023/10/03/cve-2023-4911/looney-tunables-local-privilege-escalation-glibc-ld-so.txt