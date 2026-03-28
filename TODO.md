## Ideas

### Recording

~~Recording should work even if ffmpeg is not available.~~ **Done**: the event log
is always written regardless of libav. If libav is present, mp4 is also recorded.
The event log can be replayed in-app via Shift-P.

Static linking libav is impractical: `pkg-config --static` pulls in ~30 transitive
dependencies (vpx, dav1d, aom, x264, x265, opus, lame, etc.). The current dynamic
linking with graceful fallback is the right approach.


### Internationalization

Let's make this multilingual, with a language setting popup on first run right
before the tutorial, and language dropdown in the settings panel, for as many
languages as you can provide translations for. And let's make the UI play nicely,
recommend fonts to use depending on the language, and urls where to get them. Free
TTFs please, akin to what we're using now for the UI.

### Explorer

A drop-down list of topics or scenarios to explore as mini tutorials. Same format
as event log files with addition of dialog / prompt programming. Can be at top of
UI panel above Field/particle section as "Explore", will have a 'load' button,
export template based on current state button, and the load button brings up the
canonical directory where scenarios are stored. 
