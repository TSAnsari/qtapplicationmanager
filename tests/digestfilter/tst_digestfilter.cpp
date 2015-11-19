/****************************************************************************
**
** Copyright (C) 2015 Pelagicore AG
** Contact: http://www.qt.io/ or http://www.pelagicore.com/
**
** This file is part of the Pelagicore Application Manager.
**
** $QT_BEGIN_LICENSE:GPL3-PELAGICORE$
** Commercial License Usage
** Licensees holding valid commercial Pelagicore Application Manager
** licenses may use this file in accordance with the commercial license
** agreement provided with the Software or, alternatively, in accordance
** with the terms contained in a written agreement between you and
** Pelagicore. For licensing terms and conditions, contact us at:
** http://www.pelagicore.com.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3 requirements will be
** met: http://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
** SPDX-License-Identifier: GPL-3.0
**
****************************************************************************/

#include <QtCore>
#include <QtTest>

#include "digestfilter.h"

/*
 # create 512 bytes of random data
 $ dd if=/dev/urandom bs=512 count=1 >message.bin

 # create C-formatted hexdump (rawMessage below)
 $ xxd -i -c16 <message.bin

 # calculate digests
 $ sha1sum message.bin
 $ sha256sum message.bin
 $ openssl dgst -sha1 -hmac "This is secret\!" <message.bin
 $ openssl dgst -sha256 -hmac "This is secret\!" <message.bin
 */
static const unsigned char rawMessage[] = {
    0xdb, 0x99, 0xa5, 0xde, 0x50, 0x06, 0x53, 0xe2, 0x7d, 0x6c, 0xb8, 0xd5, 0xf1, 0xca, 0x58, 0xb1,
    0x90, 0x42, 0x2a, 0x0a, 0xd6, 0x31, 0xad, 0xbf, 0xf0, 0xc0, 0x7c, 0xb2, 0x3d, 0xfc, 0x0c, 0x6f,
    0xd2, 0x48, 0xc7, 0xb8, 0xf8, 0x2a, 0x5e, 0x7b, 0xb2, 0x67, 0xd3, 0xd8, 0x52, 0x2c, 0xd0, 0x7c,
    0xd2, 0xe7, 0xf2, 0x0e, 0x02, 0x24, 0x4b, 0x30, 0x3b, 0x5e, 0xfc, 0x11, 0xb5, 0xd4, 0xf7, 0x89,
    0x9a, 0x43, 0x1e, 0x8c, 0x69, 0x90, 0x17, 0xad, 0xb0, 0x15, 0xf6, 0x4f, 0xb2, 0x77, 0x09, 0xbf,
    0x93, 0x80, 0xd3, 0x7b, 0xf4, 0xa1, 0x96, 0xe0, 0x20, 0xe5, 0x7f, 0x7b, 0x07, 0xfb, 0x65, 0x41,
    0x27, 0x88, 0xf8, 0x23, 0x1d, 0x18, 0xbb, 0x2b, 0x41, 0x4d, 0xd7, 0x25, 0xe2, 0x67, 0x8c, 0xcb,
    0x12, 0xe1, 0xdf, 0x91, 0xe6, 0x24, 0x30, 0x13, 0x4c, 0x1c, 0x63, 0xe6, 0x52, 0xa5, 0x9c, 0x65,
    0xa7, 0xd4, 0x8e, 0x6d, 0x73, 0xb5, 0xe2, 0x68, 0xc8, 0x82, 0x22, 0x4b, 0xb8, 0x49, 0xd9, 0xae,
    0x90, 0xa6, 0x13, 0x3c, 0x31, 0x54, 0x22, 0x21, 0xe5, 0x96, 0x05, 0xb1, 0x4d, 0x22, 0xf6, 0x22,
    0x26, 0xed, 0x38, 0x72, 0x7e, 0x24, 0x91, 0x86, 0xde, 0x2d, 0x89, 0xad, 0x79, 0x48, 0x7b, 0x43,
    0xfb, 0x3d, 0x2c, 0x11, 0x68, 0x2c, 0x27, 0x1f, 0x2e, 0x16, 0xf7, 0x41, 0xee, 0xe3, 0xa4, 0xac,
    0xa9, 0x8d, 0xdc, 0x1a, 0xb4, 0x6b, 0x58, 0x52, 0xfa, 0x3a, 0x32, 0x05, 0x89, 0x5e, 0x91, 0x8e,
    0xb5, 0xdd, 0xd5, 0x7b, 0x27, 0x76, 0xc6, 0x40, 0x30, 0xca, 0xcb, 0x33, 0xd9, 0xed, 0x9e, 0xb9,
    0x85, 0xf7, 0x79, 0xae, 0xe2, 0xb1, 0x1f, 0xe9, 0x70, 0xaa, 0x69, 0xf5, 0x02, 0x37, 0x12, 0x2d,
    0x0c, 0x29, 0x56, 0xde, 0x44, 0xbc, 0x30, 0x1d, 0x48, 0x69, 0x23, 0x84, 0xac, 0xa8, 0xe2, 0x47,
    0x22, 0x43, 0x3f, 0xe4, 0x22, 0x55, 0x6c, 0x58, 0x50, 0x40, 0x71, 0xf4, 0xbc, 0xb4, 0x1e, 0xe8,
    0x35, 0x6e, 0xda, 0xa4, 0x75, 0x82, 0x9a, 0x69, 0xcf, 0xe1, 0x2b, 0x41, 0xfb, 0xea, 0x71, 0x13,
    0x25, 0x58, 0x6e, 0x33, 0x4c, 0x0b, 0x09, 0x18, 0x4e, 0xf6, 0x76, 0x8e, 0x9e, 0x2e, 0xaa, 0x7c,
    0xd9, 0x71, 0x89, 0xfc, 0x78, 0x1c, 0xe1, 0x24, 0x6a, 0xbc, 0x30, 0x68, 0x59, 0xa3, 0x66, 0x1a,
    0x42, 0xac, 0x19, 0x62, 0xa6, 0x22, 0xc9, 0x36, 0x8c, 0x6b, 0x78, 0x3f, 0x28, 0x4c, 0x36, 0x3a,
    0xdd, 0x60, 0xed, 0x2f, 0xa2, 0xc4, 0xd9, 0x24, 0x27, 0xb3, 0x01, 0x66, 0x26, 0xe8, 0xfa, 0xd9,
    0xf5, 0x08, 0x30, 0x45, 0x11, 0xb7, 0x78, 0x5d, 0xb7, 0x10, 0x60, 0x06, 0x3e, 0x31, 0x1e, 0x68,
    0x7c, 0x47, 0x3b, 0x23, 0xca, 0xae, 0x5a, 0xae, 0x0d, 0x1d, 0xa7, 0x2d, 0x9d, 0x16, 0x9a, 0x3d,
    0xad, 0x85, 0xc5, 0x3f, 0x62, 0x8d, 0x22, 0x71, 0x12, 0x7d, 0x51, 0xe8, 0xba, 0x72, 0x0e, 0x5c,
    0xad, 0xad, 0x39, 0xa3, 0xa3, 0x14, 0xc4, 0x49, 0xe5, 0x8a, 0x7d, 0xb1, 0xb4, 0xf9, 0x80, 0xbb,
    0x08, 0x6b, 0x48, 0xdd, 0x44, 0xc5, 0x27, 0x66, 0x4d, 0x4d, 0xc8, 0xc6, 0xd5, 0xa4, 0xac, 0x17,
    0x1f, 0xa5, 0x53, 0x67, 0x68, 0xc0, 0xb6, 0xfc, 0x2e, 0x10, 0x18, 0xe0, 0xa6, 0x79, 0x7a, 0x5c,
    0x03, 0xce, 0xf4, 0x76, 0x48, 0x41, 0x55, 0x32, 0xa8, 0x26, 0x20, 0x4a, 0x93, 0x2e, 0x9f, 0xee,
    0x2e, 0xde, 0x03, 0x6e, 0x51, 0x94, 0xc6, 0x5d, 0xe5, 0xb9, 0xae, 0x6d, 0x9c, 0x47, 0x89, 0xf5,
    0x1c, 0xb0, 0x01, 0xd0, 0x5e, 0xf5, 0x83, 0x95, 0x8d, 0x78, 0x42, 0xb7, 0x6b, 0x4e, 0xfe, 0x72,
    0x00, 0x07, 0x90, 0x4c, 0x2b, 0xa7, 0x2a, 0xda, 0x32, 0xdd, 0xfd, 0x05, 0x05, 0x0c, 0x0b, 0x45
};

class tst_DigestFilter : public QObject
{
    Q_OBJECT

public:
    tst_DigestFilter();

private slots:
    void algorithms_data();
    void algorithms();
    void misc();
    void check_data();
    void check();

private:
    QByteArray m_message;
};

Q_DECLARE_METATYPE(DigestFilter::Type)

tst_DigestFilter::tst_DigestFilter()
{
    m_message = QByteArray::fromRawData(reinterpret_cast<const char *>(rawMessage), sizeof(rawMessage));
}

void tst_DigestFilter::algorithms_data()
{
    QTest::addColumn<DigestFilter::Type>("type");
    QTest::addColumn<int>("size");
    QTest::addColumn<QString>("name");

    QTest::newRow("sha1") << DigestFilter::Sha1 << 20 << "SHA1";
    QTest::newRow("sha256") << DigestFilter::Sha256 << 32 << "SHA256";
    QTest::newRow("sha512") << DigestFilter::Sha512 << 64 << "SHA512";
}


void tst_DigestFilter::algorithms()
{
    QFETCH(DigestFilter::Type, type);
    QFETCH(int, size);
    QFETCH(QString, name);

    QCOMPARE(DigestFilter(type).size(), size);
    QCOMPARE(DigestFilter::nameFromType(type), name);
    bool ok;
    QCOMPARE(DigestFilter::typeFromName(name, &ok), type);
    QVERIFY(ok);
}

void tst_DigestFilter::misc()
{
    bool ok;
    QVERIFY(DigestFilter::typeFromName("foo", &ok) == DigestFilter::Sha1);
    QVERIFY(!ok);
    QVERIFY(DigestFilter::typeFromName("foo") == DigestFilter::Sha1);

    {
        DigestFilter df(DigestFilter::Sha1);
        QVERIFY(df.start());
        QVERIFY(!df.start());
        QVERIFY(df.errorString().contains("already start"));
    }
    {
        DigestFilter df(DigestFilter::Sha1);
        QVERIFY(!df.processData(QByteArray()));
        QVERIFY(df.errorString().contains("not start"));
        QVERIFY(df.start());
        QVERIFY(df.processData(QByteArray()));
        const char *foo = "A";
        QVERIFY(df.processData(foo, 0));
    }
    {
        DigestFilter df(DigestFilter::Sha1);
        QByteArray digest;
        QVERIFY(!df.finish(digest));
        QVERIFY(df.errorString().contains("not start"));
    }
    {
        DigestFilter df((DigestFilter::Type) -1);
        QVERIFY(!df.start());
        QVERIFY(df.errorString().contains("invalid"));
        QCOMPARE(df.size(), 0);
        QVERIFY(DigestFilter::nameFromType(df.type()).isEmpty());
    }
    QVERIFY(DigestFilter::digest((DigestFilter::Type) -1, QByteArray()).isEmpty());
    QVERIFY(!DigestFilter::digest(DigestFilter::Sha1, QByteArray()).isEmpty());
    QVERIFY(HMACFilter::hmac((DigestFilter::Type) -1, QByteArray(), QByteArray()).isEmpty());
    QVERIFY(!HMACFilter::hmac(DigestFilter::Sha1, QByteArray(), QByteArray("foo")).isEmpty());

    {
        HMACFilter hf(DigestFilter::Sha1, QByteArray());
        QVERIFY(hf.start());
        QVERIFY(hf.processData(QByteArray("foo")));
    }
}

void tst_DigestFilter::check_data()
{
    QTest::addColumn<DigestFilter::Type>("type");
    QTest::addColumn<QByteArray>("hmacKey");
    QTest::addColumn<QByteArray>("digest");

    QByteArray noHmac;
    QByteArray hmacKey = "This is secret!";

    QTest::newRow("sha1") << DigestFilter::Sha1 << noHmac << QByteArray("ca978dee32012e583b9ae1510847de09aeec098b");
    QTest::newRow("sha256") << DigestFilter::Sha256 << noHmac << QByteArray("18b4b66a8cf26169d805284c42a863f5ac52bb67317bf39dad0b1571590b52a7");
    QTest::newRow("hmac-sha1") << DigestFilter::Sha1 << hmacKey << QByteArray("e970268cbd1177793efb571cf6c38ff1875a514e");
    QTest::newRow("hmac-sha256") << DigestFilter::Sha256 << hmacKey << QByteArray("507c1d3e3bb27cb3995f1340f06be573c3db6be0f0c86f98d3804ddc38faf290");

    // these keys are double the size of the underlying digest, since they are valid hexdumped digests
    QTest::newRow("hmac-sha1-bigkey") << DigestFilter::Sha1 << QByteArray("e970268cbd1177793efb571cf6c38ff1875a514e") << QByteArray("bede9cd2c66b01c3bc184dd53c208ae2266f9b4b");
    QTest::newRow("hmac-sha256-bigkey") << DigestFilter::Sha256 << QByteArray("507c1d3e3bb27cb3995f1340f06be573c3db6be0f0c86f98d3804ddc38faf290") << QByteArray("0a9cfd1e173f8301d5b2ec6b5be82f1ceb9f5ae8f535ca37941550886bc7c16e");
}

void tst_DigestFilter::check()
{
    QFETCH(DigestFilter::Type, type);
    QFETCH(QByteArray, hmacKey);
    QFETCH(QByteArray, digest);
    QByteArray result;

    digest = QByteArray::fromHex(digest);

    // block test
    if (hmacKey.isNull())
        result = DigestFilter::digest(type, m_message);
    else
        result = HMACFilter::hmac(type, hmacKey, m_message);

    QVERIFY(result == digest);
    QCOMPARE(result.size(), DigestFilter(type).size());

    // stream test
    DigestFilter plainDigest(type);
    HMACFilter hmacDigest(type, hmacKey);
    DigestFilter *df = hmacKey.isNull() ? &plainDigest : &hmacDigest;

    result.clear();
    int chunkSize = m_message.size() / 4;
    QCOMPARE(chunkSize * 4, m_message.size());

    QVERIFY(df->start());
    QVERIFY(df->processData(m_message.left(chunkSize)));
    QVERIFY(df->processData(m_message.mid(chunkSize, chunkSize)));
    QVERIFY(df->processData(m_message.mid(2 * chunkSize, chunkSize)));
    QVERIFY(df->processData(m_message.right(chunkSize)));
    QVERIFY(df->finish(result));

    QVERIFY(result == digest);
    QCOMPARE(result.size(), DigestFilter(type).size());
}

QTEST_APPLESS_MAIN(tst_DigestFilter)

#include "tst_digestfilter.moc"

