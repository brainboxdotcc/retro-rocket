\page INKEY INKEY$ Function
```basic
INKEY$
```
Returns the key which has been pressed, or if no key has been pressed, an empty string. Note that this function will use the keyboard buffer, so if multiple keys have been pressed between this call and the last call to `INKEY$` (or an `INPUT` statement) it will require multiple calls to the function to receive all of the keypresses.

The following keys on the keyboard are designated as 'extended keys' and will return the following character codes from `INKEY$`:

<table>
<tr><th>Key</th><th>INKEY$ value</th></tr>
<tr><td>PAGEUP</td><td>245</td></tr>
<tr><td>PAGEDOWN</td><td>246</td></tr>
<tr><td>DEL</td><td>47</td></tr>
<tr><td>INS</td><td>48</td></tr>
<tr><td>END</td><td>49</td></tr>
<tr><td>UP</td><td>250</td></tr>
<tr><td>DOWN</td><td>251</td></tr>
<tr><td>LEFT</td><td>252</td></tr>
<tr><td>RIGHT</td><td>253</td></tr>
<tr><td>HOME</td><td>254</td></tr>
</table>

