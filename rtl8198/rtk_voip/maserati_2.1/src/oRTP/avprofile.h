/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc1889) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef AVPROFILE_H
#define AVPROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

int get_list_number_with_pt(int payload);
int get_codec_name_with_pt(int payload, char *codecname);
void AddRtpProfilePayload(RtpProfile *profile);
int dynamic_pt_get(RtpPayloadType pt);
int dynamic_pt_set(RtpPayloadType pt, char pt_value);
int dynamic_pt_check(void);
void av_profile_init(RtpProfile *profile);
void av_profile_destroy(RtpProfile *profile);
int get_E8C_codec_name_with_pt(int payload,char * codecname);

#ifdef __cplusplus
}
#endif

#endif //#ifndef AVPROFILE_H
