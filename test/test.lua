require 'sndfile'

local eps = 1e-8

local function testfile(filename)
   print('read file...')
   local f = sndfile.SndFile(filename)
   local x = f:readFloat(f:info().frames)

   print('write into sound buffer...')
   local s = torch.ByteStorage()
   local g = sndfile.SndFile(s, 'w', {samplerate=16000, channels=1, format='WAV', subformat='FLOAT'})
   g:writeFloat(x)
   g:close()

   print('write buffer to file...')
   local h = torch.DiskFile('export.wav', 'w'):binary()
   h:writeByte(s)
   h:close()

   print('read written buffer...')
   local h = sndfile.SndFile('export.wav')
   print(h:info())
   local y = h:readFloat(h:info().frames)
   assert((x-y):norm() <= eps)

   print('read into storage buffer...')
   local f = torch.DiskFile(filename)
   f:seekEnd()
   local size = f:position()
   f:seek(1)
   local s = f:readByte(size-1)
   f:close()

   print('read from buffer...')
   local f = sndfile.SndFile(s)
   print(f:info())
   local z = f:readFloat(f:info().frames)
   assert((x-z):norm() <= eps)
end

testfile('rock.wav')
testfile('wealth.flac')

print('PASSED')
