# sndfile: an interface to libsndfile

The package provide a interface to the [libsndfile](http://www.mega-nerd.com/libsndfile/) library.

## Installation

``` sh
luarocks install sndfile
```

## Example
``` lua
> require 'sndfile'
> f = sndfile.SndFile('blah.htk') -- open HTK file
> print(f:info())
{[samplerate] = 16000
 [sections]   = 1
 [subformat]  = string : "PCM16"
 [channels]   = 1
 [seekable]   = true
 [format]     = string : "HTK"
 [frames]     = 49460
 [endian]     = string : "FILE"}
> x = f:readShort(f:info().frames) -- read all frames
> print(x:size()) -- note: there is only one channel

 49460
     1
[torch.LongStorage of size 2]
>
> -- ok, now write the stuff in WAV
> f = sndfile.SndFile('blah.wav', 'w', {samplerate=16000, channels=1, format="WAV", subformat="PCM16"})
> f:string('title', 'dude it is awesome') -- write some title in there
> f:writeShort(x)
> f:close() -- done
```

For more details, you can check [libsndfile documentation](http://www.mega-nerd.com/libsndfile/).
For supported formats, see sndfile.formatlist(), sndfile.subformatlist() and sndfile.endianlist().

