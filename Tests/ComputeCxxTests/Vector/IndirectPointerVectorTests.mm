#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#include "Containers/IndirectPointerVector.h"
#include "Data/Table.h"

@interface IndirectPointerVectorTests : XCTestCase
@end

@implementation IndirectPointerVectorTests

- (void)testInit {
    auto vector = AG::indirect_pointer_vector<uint64_t>();
    XCTAssertTrue(vector.empty());
    XCTAssertEqual(vector.size(), 0);

    XCTAssert(vector.begin() == vector.end());

    for (auto element : vector) {
        XCTFail(@"New vector should be empty");
    }
}

- (void)testClearWhenEmpty {
    auto vector = AG::indirect_pointer_vector<uint64_t>();

    vector.clear();
}

- (void)testOneElement {
    uint64_t testElement = 1;

    auto vector = AG::indirect_pointer_vector<uint64_t>();
    vector.push_back(&testElement);

    XCTAssertFalse(vector.empty());
    XCTAssertEqual(vector.size(), 1);

    NSInteger i = 0;
    for (auto element : vector) {
        if (i == 0) {
            XCTAssertEqual(element, &testElement);
        } else {
            XCTFail(@"Vector contains too many elements");
        }
        ++i;
    }

    vector.erase(vector.begin());
    XCTAssertTrue(vector.empty());
    XCTAssertEqual(vector.size(), 0);
}

- (void)testTwoElements {
    uint64_t testElement1 = 1;
    uint64_t testElement2 = 1;

    auto vector = AG::indirect_pointer_vector<uint64_t>();
    vector.push_back(&testElement1);
    vector.push_back(&testElement2);

    XCTAssertFalse(vector.empty());
    XCTAssertEqual(vector.size(), 2);

    NSInteger i = 0;
    for (auto element : vector) {
        if (i == 0) {
            XCTAssertEqual(element, &testElement1);
        } else if (i == 1) {
            XCTAssertEqual(element, &testElement2);
        } else {
            XCTFail(@"Vector contains too many elements");
        }
        ++i;
    }

    vector.erase(vector.begin());
    vector.erase(vector.begin());
    XCTAssertTrue(vector.empty());
    XCTAssertEqual(vector.size(), 0);
}

@end
