/***************************************************************
 *
 * Copyright (C) 2024, Pelican Project, Morgridge Institute for Research
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/

#pragma once

#include "TokenFile.hh"

#include <map>
#include <memory>
#include <string>

class XrdSysError;

class HTTPRequest {
  public:
	HTTPRequest(const std::string &hostUrl, XrdSysError &log,
				const TokenFile *token)
		: hostUrl(hostUrl), m_log(log), m_token(token) {
		// Parse the URL and populate
		// What to do if the function returns false?
		// TODO: Figure out best way to deal with this
		if (!parseProtocol(hostUrl, protocol)) {
			errorCode = "E_INVALID_HOST_URL";
			errorMessage = "Failed to parse protocol from host/service URL.";
		}
	}
	virtual ~HTTPRequest();

	virtual const std::string *getAccessKey() const { return nullptr; }
	virtual const std::string *getSecretKey() const { return nullptr; }

	virtual bool parseProtocol(const std::string &url, std::string &protocol);

	virtual bool SendHTTPRequest(const std::string &payload);

	unsigned long getResponseCode() const { return responseCode; }
	const std::string &getErrorCode() const { return errorCode; }
	const std::string &getErrorMessage() const { return errorMessage; }
	const std::string &getResultString() const { return resultString; }

	// Currently only used in PUTS, but potentially useful elsewhere
	struct Payload {
		const std::string *data;
		size_t sentSoFar;
	};

	// Initialize libraries for HTTP.
	//
	// Should be called at least once per application from a non-threaded
	// context.
	static void init();

  protected:
	bool sendPreparedRequest(const std::string &protocol,
							 const std::string &uri,
							 const std::string &payload);

	typedef std::map<std::string, std::string> AttributeValueMap;
	AttributeValueMap query_parameters;
	AttributeValueMap headers;

	std::string hostUrl;
	std::string protocol;

	bool requiresSignature{false};
	struct timespec signatureTime;

	std::string errorMessage;
	std::string errorCode;

	std::string resultString;
	unsigned long responseCode{0};
	unsigned long expectedResponseCode = 200;
	bool includeResponseHeader{false};

	std::string httpVerb{"POST"};
	std::unique_ptr<HTTPRequest::Payload> callback_payload;

	XrdSysError &m_log;

  private:
	const TokenFile *m_token;
};

class HTTPUpload : public HTTPRequest {
  public:
	HTTPUpload(const std::string &h, const std::string &o, XrdSysError &log,
			   const TokenFile *token)
		: HTTPRequest(h, log, token), object(o) {
		hostUrl = hostUrl + "/" + object;
	}

	virtual ~HTTPUpload();

	virtual bool SendRequest(const std::string &payload, off_t offset,
							 size_t size);

  protected:
	std::string object;
	std::string path;
};

class HTTPDownload : public HTTPRequest {
  public:
	HTTPDownload(const std::string &h, const std::string &o, XrdSysError &log,
				 const TokenFile *token)
		: HTTPRequest(h, log, token), object(o) {
		hostUrl = hostUrl + "/" + object;
	}

	virtual ~HTTPDownload();

	virtual bool SendRequest(off_t offset, size_t size);

  protected:
	std::string object;
};

class HTTPHead : public HTTPRequest {
  public:
	HTTPHead(const std::string &h, const std::string &o, XrdSysError &log,
			 const TokenFile *token)
		: HTTPRequest(h, log, token), object(o) {
		hostUrl = hostUrl + "/" + object;
	}

	virtual ~HTTPHead();

	virtual bool SendRequest();

  protected:
	std::string object;
};
