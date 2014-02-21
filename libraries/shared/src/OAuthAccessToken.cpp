//
//  OAuthAccessToken.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDataStream>

#include "OAuthAccessToken.h"

OAuthAccessToken::OAuthAccessToken() :
    token(),
    refreshToken(),
    expiryTimestamp(),
    tokenType()
{
    
}

OAuthAccessToken::OAuthAccessToken(const QJsonObject& jsonObject) :
    token(jsonObject["access_token"].toString()),
    refreshToken(jsonObject["refresh_token"].toString()),
    expiryTimestamp(QDateTime::currentMSecsSinceEpoch() + jsonObject["expires_in"].toInt()),
    tokenType(jsonObject["token_type"].toString())
{
    
}

OAuthAccessToken::OAuthAccessToken(const OAuthAccessToken& otherToken) {
    token = otherToken.token;
    refreshToken = otherToken.refreshToken;
    expiryTimestamp = otherToken.expiryTimestamp;
    tokenType = otherToken.tokenType;
}

OAuthAccessToken& OAuthAccessToken::operator=(const OAuthAccessToken& otherToken) {
    OAuthAccessToken temp(otherToken);
    swap(temp);
    return *this;
}

void OAuthAccessToken::swap(OAuthAccessToken& otherToken) {
    using std::swap;
    
    swap(token, otherToken.token);
    swap(refreshToken, otherToken.refreshToken);
    swap(expiryTimestamp, otherToken.expiryTimestamp);
    swap(tokenType, otherToken.tokenType);
}

QDataStream& operator<<(QDataStream &out, const OAuthAccessToken& token) {
    out << token.token << token.expiryTimestamp << token.tokenType << token.refreshToken;
    return out;
}

QDataStream& operator>>(QDataStream &in, OAuthAccessToken& token) {
    in >> token.token >> token.expiryTimestamp >> token.tokenType >> token.refreshToken;
    return in;
}
